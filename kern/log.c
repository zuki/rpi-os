#include "types.h"
#include "log.h"
#include "console.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "vfs.h"
#include "v6.h"
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
    int flag;                   // LOGENABLED/
    struct logheader lh;
};
struct log log[NLOG];

#define LOGENABLED  1

extern struct superblock sb[NMINOR];

static void recover_from_log();
static void commit(int dev);

// log機能はV6固有だが、begin_op()する段階ではどのFSを使うか
// わからないのでログを区別できない。速度的には無駄になるが
// log_write()はデバイス判定できるので実害はないと思われる
void
initlog(int dev)
{
    // ログヘッダはディスクのログ領域の先頭ブロックにある
    // だから、1ブロック未満でなければならない
    if (sizeof(struct logheader) >= sb[dev].blocksize)
        panic("initlog: too big logheader");
    initlock(&log[dev].lock, "log");
    struct v6_superblock *v6sb = sb[dev].fs_info;
    log[dev].start = v6sb->logstart;
    log[dev].size = v6sb->nlog;
    log[dev].dev = dev;
    log[dev].flag = LOGENABLED;
    recover_from_log();
    info("init log ok");
}

/* コミット済みブロックをディスクから本来のblockにコピー */
static void
install_trans(int dev)
{
    if (dev >= NLOG || !(log[dev].flag & LOGENABLED)) return;

    for (int tail = 0; tail < log[dev].lh.n; tail++) {
        struct buf *lbuf = bread(log[dev].dev, log[dev].start + tail + 1);    // read log block（先頭はheaderなので+1)
        struct buf *dbuf = bread(log[dev].dev, log[dev].lh.block[tail]);      // read dst
        memmove(dbuf->data, lbuf->data, BSIZE);                     // copy block to dst
        bwrite(dbuf);                                               // write dst to disk
        brelse(lbuf);
        brelse(dbuf);
    }
}

/* ログヘッダをディスクからメモリに読み込む */
static void
read_head(int dev)
{
    if (dev >= NLOG || !(log[dev].flag & LOGENABLED)) return;

    struct buf *buf = bread(log[dev].dev, log[dev].start);  // log headerを読み込み
    struct logheader *lh = (struct logheader *)(buf->data);
    log[dev].lh.n = lh->n;
    for (int i = 0; i < log[dev].lh.n; i++) {
        log[dev].lh.block[i] = lh->block[i];
    }
    brelse(buf);
}

/*
 * メモリ上のログヘッダをディスクに書き込む。
 * これが本当の意味でのカレントトランザクションの
 * コミッションポイント
 */
static void
write_head(int dev)
{
    if (dev >= NLOG || !(log[dev].flag & LOGENABLED)) return;

    struct buf *buf = bread(log[dev].dev, log[dev].start);
    struct logheader *hb = (struct logheader *)(buf->data);
    hb->n = log[dev].lh.n;
    for (int i = 0; i < log[dev].lh.n; i++) {
        hb->block[i] = log[dev].lh.block[i];
    }
    bwrite(buf);
    brelse(buf);
}

static void
recover_from_log()
{
    for (int i = 0; i < NLOG; i++) {
        read_head(i);
        install_trans(i);            // if committed, copy from log to disk
        log[i].lh.n = 0;
        write_head(i);               // clear the log
    }
}

/* FSシステムコールを開始する際に呼ばれる */
void
begin_op()
{
    for (int i = 0; i < NLOG; i++) {
        if (!log[i].flag & LOGENABLED) continue;

        acquire(&log[i].lock);
        while (1) {
            if (log[i].committing) {
                sleep(&log[i], &log[i].lock);
            } else if (log[i].lh.n + (log[i].outstanding + 1) * MAXOPBLOCKS >
                    LOGSIZE) {
                // ログが枯渇する恐れがあるのでコミットされるのを待機する
                sleep(&log[i], &log[i].lock);
            } else {
                log[i].outstanding += 1;
                release(&log[i].lock);
                break;
            }
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
    for (int i = 0; i < NLOG; i++) {
        if (!log[i].flag & LOGENABLED) continue;

        int do_commit = 0;

        acquire(&log[i].lock);
        log[i].outstanding -= 1;
        if (log[i].committing)
            panic("log committing");
        if (log[i].outstanding == 0) {
            do_commit = 1;
            log[i].committing = 1;
        } else {
            // begin_op()がログスペースが空くのを待っている可能性がある。
            // また、log[i].outstandingを減ずることで予約スペースが減じた可能性がある
            wakeup(&log[i]);
        }
        release(&log[i].lock);

        if (do_commit) {
            // commit()はロックせずに実行する。なぜなら、
            // ロックも持ってsleepすることが許されないから。
            commit(i);
            acquire(&log[i].lock);
            log[i].committing = 0;
            wakeup(&log[i]);
            release(&log[i].lock);
        }
    }

}

/* 変更されたブロックをキャッシュからログへコピーする */
static void
write_log(int dev)
{
    if (dev >= NLOG || !(log[dev].flag & LOGENABLED)) return;

    for (int tail = 0; tail < log[dev].lh.n; tail++) {
        struct buf *to = bread(log[dev].dev, log[dev].start + tail + 1);  // log block
        struct buf *from = bread(log[dev].dev, log[dev].lh.block[tail]);  // cache block
        memmove(to->data, from->data, BSIZE);
        bwrite(to);                                             // write the log
        brelse(from);
        brelse(to);
    }
}

static void
commit(int dev)
{
    if (dev >= NLOG || !(log[dev].flag & LOGENABLED)) return;

    if (log[dev].lh.n > 0) {
        write_log(dev);            // Write modified blocks from cache to log
        write_head(dev);           // Write header to disk -- the real commit
        install_trans(dev);        // Now install writes to home locations
        disb();
        log[dev].lh.n = 0;
        disb();
        write_head(dev);           // Erase the transaction from the log
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
    if (b->dev >= NLOG || !log[b->dev].flag & LOGENABLED) return;

    int i;

    if (log[b->dev].lh.n >= LOGSIZE || log[b->dev].lh.n >= log[b->dev].size - 1)
        panic("too big a transaction");
    if (log[b->dev].outstanding < 1)
        panic("log_write outside of trans: dev: %d, blockno: %d, outstanding: %d\n",
            b->dev, b->blockno, log[b->dev].outstanding);

    acquire(&log[b->dev].lock);
    for (i = 0; i < log[b->dev].lh.n; i++) {
        if (log[b->dev].lh.block[i] == b->blockno)      // すでにログにあればそのまま使う
            break;
    }
    log[b->dev].lh.block[i] = b->blockno;
    if (i == log[b->dev].lh.n)                          // なかった場合は新たに追加
        log[b->dev].lh.n++;
    b->flags |= B_DIRTY;                        // prevent eviction
    release(&log[b->dev].lock);
}
