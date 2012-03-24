/* Generated from packet-show.cast by ../../util/caster.pl, do not edit. */

#include "types.h"

/* (line 4) char *show_packet_tag(ops_packet_tag_t packet_tag, packet_tag_map_t *packet_tag_map) -> char *ops_str_from_map(int packet_tag, ops_map_t *packet_tag_map) */
char *ops_str_from_map(int packet_tag, ops_map_t *packet_tag_map);
#define show_packet_tag(packet_tag,packet_tag_map) ops_str_from_map(CHECKED_INSTANCE_OF(ops_packet_tag_t , packet_tag),CHECKED_INSTANCE_OF( packet_tag_map_t *, packet_tag_map))
typedef char * show_packet_tag_t(ops_packet_tag_t , packet_tag_map_t *);

/* (line 5) char *show_sig_type(ops_sig_type_t sig_type, sig_type_map_t *sig_type_map) -> char *ops_str_from_map(int sig_type, ops_map_t *sig_type_map) */
char *ops_str_from_map(int sig_type, ops_map_t *sig_type_map);
#define show_sig_type(sig_type,sig_type_map) ops_str_from_map(CHECKED_INSTANCE_OF(ops_sig_type_t , sig_type),CHECKED_INSTANCE_OF( sig_type_map_t *, sig_type_map))
typedef char * show_sig_type_t(ops_sig_type_t , sig_type_map_t *);

/* (line 6) char *show_pka(ops_public_key_algorithm_t pka, public_key_algorithm_map_t *pka_map) -> char *ops_str_from_map(int pka, ops_map_t *pka_map) */
char *ops_str_from_map(int pka, ops_map_t *pka_map);
#define show_pka(pka,pka_map) ops_str_from_map(CHECKED_INSTANCE_OF(ops_public_key_algorithm_t , pka),CHECKED_INSTANCE_OF( public_key_algorithm_map_t *, pka_map))
typedef char * show_pka_t(ops_public_key_algorithm_t , public_key_algorithm_map_t *);

/* (line 7) char *show_ss_type(ops_ss_type_t ss_type, ss_type_map_t *ss_type_map) -> char *ops_str_from_map(int ss_type, ops_map_t *ss_type_map) */
char *ops_str_from_map(int ss_type, ops_map_t *ss_type_map);
#define show_ss_type(ss_type,ss_type_map) ops_str_from_map(CHECKED_INSTANCE_OF(ops_ss_type_t , ss_type),CHECKED_INSTANCE_OF( ss_type_map_t *, ss_type_map))
typedef char * show_ss_type_t(ops_ss_type_t , ss_type_map_t *);

/* (line 8) char *show_ss_rr_code(ops_ss_rr_code_t ss_rr_code, ss_rr_code_map_t *ss_rr_code_map) -> char *ops_str_from_map(int ss_rr_code, ops_map_t *ss_rr_code_map) */
char *ops_str_from_map(int ss_rr_code, ops_map_t *ss_rr_code_map);
#define show_ss_rr_code(ss_rr_code,ss_rr_code_map) ops_str_from_map(CHECKED_INSTANCE_OF(ops_ss_rr_code_t , ss_rr_code),CHECKED_INSTANCE_OF( ss_rr_code_map_t *, ss_rr_code_map))
typedef char * show_ss_rr_code_t(ops_ss_rr_code_t , ss_rr_code_map_t *);

/* (line 9) char *show_hash_algorithm(unsigned char hash,+ops_map_t *hash_algorithm_map) -> char *ops_str_from_map(int hash,ops_map_t *hash_algorithm_map) */
char *ops_str_from_map(int hash,ops_map_t *hash_algorithm_map);
#define show_hash_algorithm(hash) ops_str_from_map(CHECKED_INSTANCE_OF(unsigned char , hash),CHECKED_INSTANCE_OF(ops_map_t *, hash_algorithm_map))
typedef char * show_hash_algorithm_t(unsigned char );

/* (line 10) char *show_symmetric_algorithm(unsigned char hash,+ops_map_t *symmetric_algorithm_map) -> char *ops_str_from_map(int hash,ops_map_t *symmetric_algorithm_map) */
char *ops_str_from_map(int hash,ops_map_t *symmetric_algorithm_map);
#define show_symmetric_algorithm(hash) ops_str_from_map(CHECKED_INSTANCE_OF(unsigned char , hash),CHECKED_INSTANCE_OF(ops_map_t *, symmetric_algorithm_map))
typedef char * show_symmetric_algorithm_t(unsigned char );
