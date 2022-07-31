#include "types.h"
#include "console.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"
#include "string.h"

/* FSシステムコールの並行処理を可能にするシンプルなロギング
 * ログトランザクションは複数のFSシステムコールの更新を含んでいる。
 * ロギングシステムはアクティブなFSシステムコールが存在しない場合に
 * のみコミットする。したがって、コミットがまだコミットされていない
 * システムコールの更新をディスクに書き込むか否かについてどうかは
 * 考える必要がない。
 *
 * システムコールはその開始と終了を begin_op()/end_op()を呼び出して
 * マークする必要がある。通常、begin_op()は進行中のFSシステムコールの
 * カウントを増加させて戻るだけである。しかし、ログが枯渇しそうになると
 * 最後の未処理のend_op()がコミットするまでスリープする。
 *
 * ログはディスクブロックを含む物理的なREDOログである。ディスク上の
 * ログ形式は次のとおりである。
 *
 *   - ヘッダブロックブロック（A, B, C, ...のブロック番号を含む）
 *   - ブロックA
 *   - ブロックB
 *   - ブロックC
 *   ...
 *
 * ログの追記は同期的に行われる。
 */

/* ヘッダブロックの内容であり、ディスク上のヘッダブロックとコミット前の
 * ログブロック番号をメモリ上で追跡するために使用される、
 */
struct logheader {
    int n;
    int block[LOGSIZE];
};

struct log {
    struct spinlock lock;
    int start;
    int size;
    int outstanding;            // 実行中のFSシステムコール数
    int committing;             // commit()実行中。待て。
    int dev;
    struct logheader lh;
};
struct log log;

extern void readsb(int dev, struct superblock *sb);

static void recover_from_log();
static void commit();

void
initlog(int dev)
{
    if (sizeof(struct logheader) >= BSIZE)      // ログヘッダはディスクのログ領域の先頭ブロックにある。
        panic("initlog: too big logheader");    // だから、1ブロック未満でなければならない

    struct superblock sb;
    initlock(&log.lock, "log");
    readsb(dev, &sb);
    log.start = sb.logstart;
    log.size = sb.nlog;
    log.dev = dev;
    recover_from_log();
}

/* コミット済みブロックをディスクから本来のblockにコピー */
static void
install_trans()
{
    for (int tail = 0; tail < log.lh.n; tail++) {
        struct buf *lbuf = bread(log.dev, log.start + tail + 1);    // read log block（先頭はheaderなので+1)
        struct buf *dbuf = bread(log.dev, log.lh.block[tail]);      // read dst
        memmove(dbuf->data, lbuf->data, BSIZE);                     // copy block to dst
        bwrite(dbuf);                                               // write dst to disk
        brelse(lbuf);
        brelse(dbuf);
    }
}

/* ログヘッダをディスクからメモリに読み込む */
static void
read_head()
{
    struct buf *buf = bread(log.dev, log.start);                    // log headerを読み込み
    struct logheader *lh = (struct logheader *)(buf->data);
    int i;
    log.lh.n = lh->n;
    for (i = 0; i < log.lh.n; i++) {
        log.lh.block[i] = lh->block[i];
    }
    brelse(buf);
}

/*
 * メモリ上のログヘッダをディスクに書き込む。
 * これが本当の意味でのカレントトランザクションの
 * コミッションポイント
 */
static void
write_head()
{
    struct buf *buf = bread(log.dev, log.start);
    struct logheader *hb = (struct logheader *)(buf->data);
    int i;
    hb->n = log.lh.n;
    for (i = 0; i < log.lh.n; i++) {
        hb->block[i] = log.lh.block[i];
    }
    bwrite(buf);
    brelse(buf);
}

static void
recover_from_log()
{
    read_head();
    install_trans();            // if committed, copy from log to disk
    log.lh.n = 0;
    write_head();               // clear the log
}

/* FSシステムコールを開始する際に呼ばれる */
void
begin_op()
{
    acquire(&log.lock);
    while (1) {
        if (log.committing) {
            sleep(&log, &log.lock);
        } else if (log.lh.n + (log.outstanding + 1) * MAXOPBLOCKS >
                   LOGSIZE) {
            // ログが枯渇する恐れがあるのでコミットされるのを待機する
            sleep(&log, &log.lock);
        } else {
            log.outstanding += 1;
            release(&log.lock);
            break;
        }
    }
}

/*
 * FSシステムコールの終了時に呼ばれる。
 * これが実行中の最後のシステムコールの場合はコミットする
 */
void
end_op()
{
    int do_commit = 0;

    acquire(&log.lock);
    log.outstanding -= 1;
    if (log.committing)
        panic("log.committing");
    if (log.outstanding == 0) {
        do_commit = 1;
        log.committing = 1;
    } else {
        // begin_op()がログスペースが空くのを待っている可能性がある。
        // また、log.outstandingを減ずることで予約スペースが減じた可能性がある
        wakeup(&log);
    }
    release(&log.lock);

    if (do_commit) {
        // commit()はロックせずに実行する。なぜなら、
        // ロックも持ってsleepすることが許されないから。
        commit();
        acquire(&log.lock);
        log.committing = 0;
        wakeup(&log);
        release(&log.lock);
    }
}

/* 変更されたブロックをキャッシュからログへコピーする */
static void
write_log()
{
    for (int tail = 0; tail < log.lh.n; tail++) {
        struct buf *to = bread(log.dev, log.start + tail + 1);  // log block
        struct buf *from = bread(log.dev, log.lh.block[tail]);  // cache block
        memmove(to->data, from->data, BSIZE);
        bwrite(to);                                             // write the log
        brelse(from);
        brelse(to);
    }
}

static void
commit()
{
    if (log.lh.n > 0) {
        write_log();            // Write modified blocks from cache to log
        write_head();           // Write header to disk -- the real commit
        install_trans();        // Now install writes to home locations
        disb();
        log.lh.n = 0;
        disb();
        write_head();           // Erase the transaction from the log
    }
}

/* 呼び出し元はバッファを処理してb->dataを変更した。
 * キャッシュ内のブロック番号とピンをB_DIRTYを付けて記録する。
 * commit()/write_log()がディスクへの書き込みを行う。
 *
 * log_write()がbwrite()を置き換える; 典型的な使用法は次の通り:
 *   bp = bread(...)
 *   modify bp->data[]
 *   log_write(bp)
 *   brelse(bp)
 */
void
log_write(struct buf *b)
{
    int i;

    if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
        panic("too big a transaction");
    if (log.outstanding < 1)
        panic("log_write outside of trans");

    acquire(&log.lock);
    for (i = 0; i < log.lh.n; i++) {
        if (log.lh.block[i] == b->blockno)      // すでにログにあればそのまま使う
            break;
    }
    log.lh.block[i] = b->blockno;
    if (i == log.lh.n)                          // なかった場合は新たに追加
        log.lh.n++;
    b->flags |= B_DIRTY;                        // prevent eviction
    release(&log.lock);
}
