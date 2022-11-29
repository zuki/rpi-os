#include "usb/usb.h"
#include "usb/config_parser.h"
#include "types.h"
#include "console.h"

#define SKIP_BYTES(desc, bytes) ((usb_desc_t *) ((uint8_t *)(desc) + (bytes)))

void cfg_parser(cfg_parser_t *self, void *buffer, unsigned buflen)
{
    self->buffer    = (usb_desc_t *)buffer;
    self->buflen    = buflen;
    self->valid     = false;
    self->end_pos   = SKIP_BYTES(self->buffer, buflen);
    self->next_pos  = self->buffer;
    self->curr_desc = 0;
    self->err_pos   = self->buffer;

    if (self->buflen < 4 || self->buflen > 512) {    // total_length は必須
        warn("wrong buflen: 0x%x", self->buflen);
        return;
    }

    // 先頭はconfiguration descriptor
    if (self->buffer->config.length != sizeof(cfg_desc_t)
     || self->buffer->config.type   != DESCRIPTOR_CONFIGURATION
     || self->buffer->config.total > buflen) {
        warn("1st desc is not cfg_desc");
        return;
     }


    if (self->buffer->config.total < buflen) {
        self->end_pos = SKIP_BYTES(self->buffer, self->buffer->config.total);
    }

    const usb_desc_t *curr_pos = self->buffer;
    uint8_t last_type = 0;
    //boolean in_audio = false;
    //int loop = 0;
    while (SKIP_BYTES(curr_pos, 2) < self->end_pos) {
        //if (++loop > 4) panic("loop");
        uint8_t len = curr_pos->header.length;
        uint8_t type = curr_pos->header.type;

        usb_desc_t *end = SKIP_BYTES(curr_pos, len);
        if (end > self->end_pos) {
            self->err_pos = curr_pos;
            warn("end(0x%p) > end_pos(0x%p)", end, self->end_pos);
            return;
        }
        uint8_t expected = 0;
        switch (type) {
        case DESCRIPTOR_CONFIGURATION:
            // configurationは先頭
            if (last_type != 0) {
                warn("cfg_desc not 1st");
                self->err_pos = curr_pos;
                return;
            };
            expected = sizeof(cfg_desc_t);
            break;
        case DESCRIPTOR_INTERFACE:
            // interfaceは2番め
            if (last_type == 0) {
                warn("itf_desc is 1st");
                self->err_pos = curr_pos;
                return;
            }
            expected = sizeof(if_desc_t);
            //in_audio = (curr_pos->interface.class == 0x01);   // Audio class
            break;
        case DESCRIPTOR_ENDPOINT:
            // endpointはinterfaceより後
            if (last_type == 0 || last_type == DESCRIPTOR_CONFIGURATION) {
                warn("ep_desc is 1st or after cfg_desc");
                self->err_pos = curr_pos;
                return;
            }
            //expected = in_audio ? sizeof(audio_ep_desc_t) : sizeof(ep_desc_t);
            expected = sizeof(ep_desc_t);
            break;
        default:
            break;
        }

        if (expected != 0 && len != expected) {
            warn("len(%d) != expected(%d)", len, expected);
            self->err_pos = curr_pos;
            return;
        }

        last_type = type;
        curr_pos = end;
        //info("skip(0x%p), end_pos(0x%p)", SKIP_BYTES(curr_pos, 2), self->end_pos);
    }

    if (curr_pos != self->end_pos) {
        warn("curr_pos(0x%p) != end_pos(0x%p)", curr_pos, self->end_pos);
        self->err_pos = curr_pos;
        return;
    }

    self->valid = true;
    trace("3");
}

void cfg_parser_copy(cfg_parser_t *self, cfg_parser_t *parser)
{
    self->buffer    = parser->buffer;
    self->buflen    = parser->buflen;
    self->valid     = parser->valid;
    self->end_pos   = parser->end_pos;
    self->next_pos  = parser->next_pos;
    self->curr_desc = parser->curr_desc;
    self->err_pos   = parser->err_pos;
}

void _cfg_paser(cfg_parser_t *self)
{
    self->buffer = 0;
}

boolean cfg_parser_is_valid(cfg_parser_t *self)
{
    return self->valid;
}

const usb_desc_t *cfg_parser_get_desc(cfg_parser_t *self, uint8_t type)
{
    const usb_desc_t *result = 0;

    while(self->next_pos < self->end_pos) {
        uint8_t desclen  = self->next_pos->header.length;
        uint8_t decstype = self->next_pos->header.type;
        usb_desc_t *end = SKIP_BYTES(self->next_pos, desclen);

        if (type == DESCRIPTOR_ENDPOINT && decstype == DESCRIPTOR_INTERFACE)
            break;

        if (decstype == type) {
            result = self->next_pos;
            self->next_pos = end;
            break;
        }
        self->next_pos = end;
    }

    if (result != 0)
        self->err_pos = result;

    self->curr_desc = result;

    return result;
}

void cfg_parser_error(cfg_parser_t *self, const char *source)
{
    error("Invalid configuration descriptor (offset 0x%llx)", ((uint64_t) self->err_pos - (uint64_t) self->buffer));
}

const usb_desc_t *cfg_parser_get_curr_desc(cfg_parser_t *self)
{
    return self->curr_desc;
}
