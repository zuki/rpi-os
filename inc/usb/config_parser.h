#ifndef INC_USb_CONFIG_PARSER_H
#define INC_USb_CONFIG_PARSER_H

#include "usb/usb.h"
#include "types.h"

typedef struct usb_config_parser {
    const usb_desc_t *buffer;   ///< ディスクリプタの入ったバッファ
    unsigned    buflen;         ///< ディスクリプタの合計サイズ
    boolean     valid;          ///< ディスクリプタデータとして正しいか
    const usb_desc_t *end_pos;  ///< 最後のディスクリプタのポインタ
    const usb_desc_t *next_pos; ///< 次のディスクリプタのポインタ
    const usb_desc_t *curr_desc; ///< 現在のディスクリプタのポインタ
    const usb_desc_t *err_pos;  ///< エラーが発生したディスクリプタのポインタ
} cfg_parser_t;

void cfg_parser(cfg_parser_t *self, void *buffer, unsigned buflen);
void cfg_parser_copy(cfg_parser_t *self, cfg_parser_t *parser);
void _cfg_paser(cfg_parser_t *self);

boolean cfg_parser_is_valid(cfg_parser_t *self);

const usb_desc_t *cfg_parser_get_desc(cfg_parser_t *self, uint8_t type);

const usb_desc_t *cfg_parser_get_curr_desc(cfg_parser_t *self);

void cfg_parser_error(cfg_parser_t *self, const char *source);

#endif
