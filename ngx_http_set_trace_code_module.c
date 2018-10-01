/*
 * Author    : tangyan
 * Datetime  : 2018-04-30
 * Copyleft
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_flag_t set;
    /* ngx_str_t trace_code; */
} ngx_http_set_trace_code_loc_conf_t;


static ngx_int_t ngx_http_set_trace_code_init(ngx_conf_t *conf);
static void *ngx_http_set_trace_code_create_loc_conf(ngx_conf_t *conf);
static char *ngx_http_set_trace_code_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_str_t *ngx_gen_trace_code(ngx_http_request_t *r, ngx_str_t *des);
static ngx_table_elt_t *ngx_find_header(const char *header_name, ngx_list_t *hl);

#if 0
static ssize_t ngx_base64_encode(u_char *src, size_t sl, u_char *des, size_t dl, ngx_log_t *log);
#endif


static ngx_command_t ngx_http_set_trace_code_commands[] = {
    { ngx_string("trace_code"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_set_trace_code_loc_conf_t, set),
      NULL },

    ngx_null_command
};


static ngx_http_module_t ngx_http_set_trace_code_module_ctx = {
    NULL,
    ngx_http_set_trace_code_init,     /* postconfiguration */
    NULL,
    NULL,
    NULL,
    NULL,
    ngx_http_set_trace_code_create_loc_conf,
    ngx_http_set_trace_code_merge_loc_conf
};


ngx_module_t ngx_http_set_trace_code_module = {
    NGX_MODULE_V1,
    &ngx_http_set_trace_code_module_ctx,
    ngx_http_set_trace_code_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING,
};


static ngx_http_output_header_filter_pt ngx_http_next_header_filter;
/* static ngx_http_request_body_filter_pt ngx_http_next_request_body_filter;*/


static ngx_int_t
ngx_http_set_trace_code_header_filter(ngx_http_request_t *r)
 {
     ngx_table_elt_t                   *h, *nh;
     ngx_http_set_trace_code_loc_conf_t *slcf;

     slcf = ngx_http_get_module_loc_conf(r, ngx_http_set_trace_code_module);

     if (slcf->set != 1) {
         return ngx_http_next_header_filter(r);
     }

     nh = ngx_list_push(&r->headers_out.headers);
     if (nh == NULL) {
         return NGX_ERROR;
     }

     nh->hash = 1;
     ngx_str_set(&nh->key, "trace_code");

     h = ngx_find_header("trace_code", &r->headers_in.headers);
     ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "find trace code header: %p", (void *)h);

     if (h == NULL) {
         ngx_gen_trace_code(r, &nh->value);
     } else{
         nh->value.data = h->value.data;
         nh->value.len = h->value.len;
     }

     ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                    "set trace_code to response header: %V", &nh->value);

     return ngx_http_next_header_filter(r);
}


static ngx_table_elt_t *
ngx_find_header(const char *header_name, ngx_list_t *hl)
{
    ngx_list_part_t *lp;
    ngx_table_elt_t *data;
    ngx_uint_t i;

    lp = &hl->part;
    while (lp != NULL) {
        data = lp->elts;
        for (i=0; i<lp->nelts; i++) {
            if (ngx_strncmp(header_name, data[i].key.data, data[i].key.len) == 0) {
                return &data[i];
            }
        }
        lp = lp->next;
    }

    return NULL;
}


static ngx_str_t *
ngx_gen_trace_code(ngx_http_request_t *r, ngx_str_t *des)
{
    u_char tmp[32], *si;
    ngx_str_t src;

    time_t ss;
    ngx_msec_t sms;
    in_addr_t sa;

    ss = r->start_sec;
    sms = r->start_msec;
    sa = ((struct sockaddr_in *)r->connection->sockaddr)->sin_addr.s_addr;

    si = tmp;
    ngx_memcpy(si, &ss, sizeof(time_t));
    si += sizeof(time_t);
    ngx_memcpy(si, &sms, sizeof(ngx_msec_t));
    si += sizeof(ngx_msec_t);
    ngx_memcpy(si, &sa, sizeof(in_addr_t));
    si += sizeof(in_addr_t);
    /* ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, (const char*) pos); */

    src.data = tmp;
    src.len = (size_t)(si-tmp);

    des->data = ngx_pcalloc(r->pool, sizeof(u_char) * 32);
    des->len = 32;

    /* cl = ngx_base64_encode(src, (size_t)(si-src), pos, pl, r->connection->log); */
    ngx_encode_base64(des, &src);

    return des;
}


static ngx_int_t
ngx_http_set_trace_code_handler(ngx_http_request_t *r)
{
    ngx_table_elt_t                   *h;
    ngx_http_set_trace_code_loc_conf_t *slcf;

    slcf = ngx_http_get_module_loc_conf(r, ngx_http_set_trace_code_module);

    if (slcf->set != 1) {
        return NGX_DECLINED;
    }

    h = ngx_list_push(&r->headers_in.headers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    h->hash = 1;
    ngx_str_set(&h->key, "trace_code");

    ngx_gen_trace_code(r, &h->value);

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "set trace_code to request header: %V", &h->value);
    /* ngx_log_debug3(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "%s; %i; %V", pos, sizeof(pos), &h->value); */

    return NGX_DECLINED;
}


static ngx_int_t
ngx_http_set_trace_code_init(ngx_conf_t *conf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(conf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_set_trace_code_handler;

    /*
     * handler of setting trace_code to response headers
     */
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_set_trace_code_header_filter;

    return NGX_OK;
}


static void *
ngx_http_set_trace_code_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_set_trace_code_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_set_trace_code_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->set = NGX_CONF_UNSET;
    /* ngx_str_null(&conf->trace_code); */

    return conf;
}


static char *
ngx_http_set_trace_code_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_set_trace_code_loc_conf_t *prev = parent;
    ngx_http_set_trace_code_loc_conf_t *conf = child;

    if (conf->set == NGX_CONF_UNSET) {
        conf->set = prev->set;

        /* Todo: better way? */
        /* conf->trace_code.data = prev->trace_code.data; */
        /* conf->trace_code.len = prev->trace_code.len; */

    }

    return NGX_CONF_OK;
}


#if 0
static ssize_t
ngx_base64_encode(u_char *src, size_t sl, u_char *des, size_t dl, ngx_log_t *log)
{
    u_char tmp[48], *si;
    size_t tl, il;
    unsigned short *mask;

    u_char map[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    ngx_log_debug4(NGX_LOG_DEBUG_HTTP, log, 0, "ngx base64 param: %p, %ui, %p, %ui",
                   (void *) src, (ngx_int_t)sl, (void *)des, (ngx_int_t)dl);

    if (sl > 48) {
        return (ssize_t) -1;
    }

    if (sl % 3 == 0) {
        tl = sl;
    } else {
        tl = sl + (3 - sl % 3);
    }
    ngx_memzero(tmp, tl);

    ngx_memcpy(tmp, src, sl);
    for (si=tmp, il=1; si<tmp+tl; ) {
        mask = (unsigned short *) si;

        if (il > dl){
            return (ssize_t) -1;
        }
        *(des+il-1) = map[*mask >> 10];
        il ++;

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0, "encode char: %ui", (ngx_int_t)(*mask >> 10));

        if (il > dl){
            return (ssize_t) -1;
        }
        *(des+il-1) = map[(*mask & 0x03F0) >> 4];
        il ++;

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0, "encode char: %ui", (ngx_int_t)((*mask & 0x03F0) >> 4));

        si += 1;
        mask = (unsigned short *) si;

        if (il > dl){
            return (ssize_t) -1;
        }
        *(des+il-1) = map[(*mask & 0x0FC0) >> 6];
        il ++;

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0, "encode char: %ui", (ngx_int_t)((*mask & 0x0FC0) >> 6));

        if (il > dl){
            return (ssize_t) -1;
        }
        *(des+il-1) = map[*mask & 0x003F];
        il ++;

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0, "encode char: %ui", (ngx_int_t)(*mask & 0x003F));

        si += 2;
    }

    while (tl>sl) {
        if (il > dl){
            return (ssize_t) -1;
        }
        *(des+il-1) = '=';
        il ++;

        tl -= 1;
    }

    return il-1;
}

#endif
