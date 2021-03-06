/* Copyright (c) 2008 The Board of Trustees of The Leland Stanford Junior University */
/* Copyright (c) 2011, 2012 Open Networking Foundation */
/* Copyright (c) 2012, 2013 Big Switch Networks, Inc. */
/* See the file LICENSE.loci which should have been included in the source distribution */

/****************************************************************
 *
 * Functions related to mapping wire values to object types
 * and lengths
 *
 ****************************************************************/

#include <loci/loci.h>
#include <loci/of_message.h>

/****************************************************************
 * Top level OpenFlow message length functions
 ****************************************************************/

/**
 * Get the length of a message object as reported on the wire
 * @param obj The object to check
 * @param bytes (out) Where the length is stored
 * @returns OF_ERROR_ code
 */
void
of_object_message_wire_length_get(of_object_t *obj, int *bytes)
{
    ASSERT(OF_OBJECT_TO_WBUF(obj) != NULL);
    // ASSERT(obj is message)
    *bytes = of_message_length_get(OF_OBJECT_TO_MESSAGE(obj));
}

/**
 * Set the length of a message object as reported on the wire
 * @param obj The object to check
 * @param bytes The new length of the object
 * @returns OF_ERROR_ code
 */
void
of_object_message_wire_length_set(of_object_t *obj, int bytes)
{
    ASSERT(OF_OBJECT_TO_WBUF(obj) != NULL);
    // ASSERT(obj is message)
    of_message_length_set(OF_OBJECT_TO_MESSAGE(obj), bytes);
}

/****************************************************************
 * TLV16 type/length functions
 ****************************************************************/

/**
 * Many objects are TLVs and use uint16 for the type and length values
 * stored on the wire at the beginning of the buffer.
 */
#define TLV16_WIRE_TYPE_OFFSET 0
#define TLV16_WIRE_LENGTH_OFFSET 2

/**
 * Get the length field from the wire for a standard TLV
 * object that uses uint16 for both type and length.
 * @param obj The object being referenced
 * @param bytes (out) Where to store the length
 */

void
of_tlv16_wire_length_get(of_object_t *obj, int *bytes)
{
    uint16_t val16;
    of_wire_buffer_t *wbuf = OF_OBJECT_TO_WBUF(obj);
    ASSERT(wbuf != NULL);

    of_wire_buffer_u16_get(wbuf, 
           OF_OBJECT_ABSOLUTE_OFFSET(obj, TLV16_WIRE_LENGTH_OFFSET), &val16);
    *bytes = val16;
}

/**
 * Set the length field in the wire buffer for a standard TLV
 * object that uses uint16 for both type and length.
 * @param obj The object being referenced
 * @param bytes The length value to use
 */

void
of_tlv16_wire_length_set(of_object_t *obj, int bytes)
{
    of_wire_buffer_t *wbuf = OF_OBJECT_TO_WBUF(obj);
    ASSERT(wbuf != NULL);

    of_wire_buffer_u16_set(wbuf, 
        OF_OBJECT_ABSOLUTE_OFFSET(obj, TLV16_WIRE_LENGTH_OFFSET), bytes);
}

/**
 * Get the type field from the wire for a standard TLV object that uses
 * uint16 for both type and length.
 * @param obj The object being referenced
 * @param wire_type (out) Where to store the type
 */

static void
of_tlv16_wire_type_get(of_object_t *obj, int *wire_type)
{
    uint16_t val16;
    of_wire_buffer_t *wbuf = OF_OBJECT_TO_WBUF(obj);

    of_wire_buffer_u16_get(wbuf, OF_OBJECT_ABSOLUTE_OFFSET(obj, 
           TLV16_WIRE_TYPE_OFFSET), &val16);

    *wire_type = val16;
}

/**
 * Set the object ID based on the wire buffer for any TLV object
 * @param obj The object being referenced
 * @param id The ID value representing what should be stored.
 */

void
of_tlv16_wire_object_id_set(of_object_t *obj, of_object_id_t id)
{
    int wire_type;
    of_wire_buffer_t *wbuf = OF_OBJECT_TO_WBUF(obj);
    ASSERT(wbuf != NULL);

    wire_type = of_object_to_type_map[obj->version][id];
    ASSERT(wire_type >= 0);

    of_wire_buffer_u16_set(wbuf, 
        OF_OBJECT_ABSOLUTE_OFFSET(obj, TLV16_WIRE_TYPE_OFFSET), wire_type);

    if (wire_type == OF_EXPERIMENTER_TYPE) {
        of_extension_object_id_set(obj, id);
    }
}

/**
 * Get the object ID of an extended action
 * @param obj The object being referenced
 * @param id Where to store the object ID
 * @fixme:  This should be auto generated
 *
 * If unable to map to known extension, set id to generic "experimenter"
 */

#define OF_ACTION_EXPERIMENTER_ID_OFFSET 4
#define OF_ACTION_EXPERIMENTER_SUBTYPE_OFFSET 8


static void
extension_action_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    uint32_t exp_id;
    uint8_t *buf;

    *id = OF_ACTION_EXPERIMENTER;

    buf = OF_OBJECT_BUFFER_INDEX(obj, 0);
    
    buf_u32_get(buf + OF_ACTION_EXPERIMENTER_ID_OFFSET, &exp_id);

    switch (exp_id) {
    case OF_EXPERIMENTER_ID_BSN: {
        uint32_t subtype;
        buf_u32_get(buf + OF_ACTION_EXPERIMENTER_SUBTYPE_OFFSET, &subtype);
        switch (subtype) {
        case 1: *id = OF_ACTION_BSN_MIRROR; break;
        case 2: *id = OF_ACTION_BSN_SET_TUNNEL_DST; break;
        }
        break;
    }
    case OF_EXPERIMENTER_ID_NICIRA: {
        uint16_t subtype;
        buf_u16_get(buf + OF_ACTION_EXPERIMENTER_SUBTYPE_OFFSET, &subtype);
        switch (subtype) {
        case 18: *id = OF_ACTION_NICIRA_DEC_TTL; break;
        }
        break;
    }
    }
}

/**
 * Set wire data for extension objects, not messages.
 *
 * Currently only handles BSN mirror; ignores all others
 */

void
of_extension_object_id_set(of_object_t *obj, of_object_id_t id)
{
    uint8_t *buf = OF_OBJECT_BUFFER_INDEX(obj, 0);
    
    switch (id) {
    case OF_ACTION_BSN_MIRROR:
    case OF_ACTION_ID_BSN_MIRROR:
        buf_u32_set(buf + OF_ACTION_EXPERIMENTER_ID_OFFSET,
                    OF_EXPERIMENTER_ID_BSN);
        buf_u32_set(buf + OF_ACTION_EXPERIMENTER_SUBTYPE_OFFSET, 1);
        break;
    case OF_ACTION_BSN_SET_TUNNEL_DST:
    case OF_ACTION_ID_BSN_SET_TUNNEL_DST:
        buf_u32_set(buf + OF_ACTION_EXPERIMENTER_ID_OFFSET,
                    OF_EXPERIMENTER_ID_BSN);
        buf_u32_set(buf + OF_ACTION_EXPERIMENTER_SUBTYPE_OFFSET, 2);
        break;
    case OF_ACTION_NICIRA_DEC_TTL:
    case OF_ACTION_ID_NICIRA_DEC_TTL:
        buf_u32_set(buf + OF_ACTION_EXPERIMENTER_ID_OFFSET,
                    OF_EXPERIMENTER_ID_NICIRA);
        buf_u16_set(buf + OF_ACTION_EXPERIMENTER_SUBTYPE_OFFSET, 18);
        break;
    default:
        break;
    }
}

/**
 * Get the object ID of an extended action
 * @param obj The object being referenced
 * @param id Where to store the object ID
 * @fixme:  This should be auto generated
 *
 * If unable to map to known extension, set id to generic "experimenter"
 */

static void
extension_action_id_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    uint32_t exp_id;
    uint8_t *buf;

    *id = OF_ACTION_ID_EXPERIMENTER;

    buf = OF_OBJECT_BUFFER_INDEX(obj, 0);
    
    buf_u32_get(buf + OF_ACTION_EXPERIMENTER_ID_OFFSET, &exp_id);

    switch (exp_id) {
    case OF_EXPERIMENTER_ID_BSN: {
        uint32_t subtype;
        buf_u32_get(buf + OF_ACTION_EXPERIMENTER_SUBTYPE_OFFSET, &subtype);
        switch (subtype) {
        case 1: *id = OF_ACTION_ID_BSN_MIRROR; break;
        case 2: *id = OF_ACTION_ID_BSN_SET_TUNNEL_DST; break;
        }
        break;
    }
    case OF_EXPERIMENTER_ID_NICIRA: {
        uint16_t subtype;
        buf_u16_get(buf + OF_ACTION_EXPERIMENTER_SUBTYPE_OFFSET, &subtype);
        switch (subtype) {
        case 18: *id = OF_ACTION_ID_NICIRA_DEC_TTL; break;
        }
        break;
    }
    }
}


/**
 * Get the object ID based on the wire buffer for an action object
 * @param obj The object being referenced
 * @param id Where to store the object ID
 */


void
of_action_wire_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    int wire_type;

    of_tlv16_wire_type_get(obj, &wire_type);
    if (wire_type == OF_EXPERIMENTER_TYPE) {
        extension_action_object_id_get(obj, id);
        return;
    }

    ASSERT(wire_type >= 0 && wire_type < OF_ACTION_ITEM_COUNT);

    *id = of_action_type_to_id[obj->version][wire_type];
    ASSERT(*id != OF_OBJECT_INVALID);
}

/**
 * Get the object ID based on the wire buffer for an action ID object
 * @param obj The object being referenced
 * @param id Where to store the object ID
 */


void
of_action_id_wire_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    int wire_type;

    of_tlv16_wire_type_get(obj, &wire_type);
    if (wire_type == OF_EXPERIMENTER_TYPE) {
        extension_action_id_object_id_get(obj, id);
        return;
    }

    ASSERT(wire_type >= 0 && wire_type < OF_ACTION_ID_ITEM_COUNT);

    *id = of_action_id_type_to_id[obj->version][wire_type];
    ASSERT(*id != OF_OBJECT_INVALID);
}

/**
 * @fixme to do when we have instruction extensions
 * See extension_action above
 */

static int
extension_instruction_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    (void)obj;

    *id = OF_INSTRUCTION_EXPERIMENTER;

    return OF_ERROR_NONE;
}

/**
 * Get the object ID based on the wire buffer for an instruction object
 * @param obj The object being referenced
 * @param id Where to store the object ID
 */

void
of_instruction_wire_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    int wire_type;

    of_tlv16_wire_type_get(obj, &wire_type);
    if (wire_type == OF_EXPERIMENTER_TYPE) {
        extension_instruction_object_id_get(obj, id);
        return;
    }

    ASSERT(wire_type >= 0 && wire_type < OF_INSTRUCTION_ITEM_COUNT);

    *id = of_instruction_type_to_id[obj->version][wire_type];
    ASSERT(*id != OF_OBJECT_INVALID);
}


/**
 * @fixme to do when we have queue_prop extensions
 * See extension_action above
 */

static void
extension_queue_prop_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    (void)obj;

    *id = OF_QUEUE_PROP_EXPERIMENTER;
}

/**
 * Get the object ID based on the wire buffer for an queue_prop object
 * @param obj The object being referenced
 * @param id Where to store the object ID
 */

void
of_queue_prop_wire_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    int wire_type;

    of_tlv16_wire_type_get(obj, &wire_type);
    if (wire_type == OF_EXPERIMENTER_TYPE) {
        extension_queue_prop_object_id_get(obj, id);
        return;
    }

    ASSERT(wire_type >= 0 && wire_type < OF_QUEUE_PROP_ITEM_COUNT);

    *id = of_queue_prop_type_to_id[obj->version][wire_type];
    ASSERT(*id != OF_OBJECT_INVALID);
}


/**
 * @fixme to do when we have table_feature_prop extensions
 * See extension_action above
 */

static void
extension_table_feature_prop_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    (void)obj;

    *id = OF_TABLE_FEATURE_PROP_EXPERIMENTER;
}

/**
 * Table feature property object ID determination
 *
 * @param obj The object being referenced
 * @param id Where to store the object ID
 */

void
of_table_feature_prop_wire_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    int wire_type;

    of_tlv16_wire_type_get(obj, &wire_type);
    if (wire_type == OF_EXPERIMENTER_TYPE) {
        extension_table_feature_prop_object_id_get(obj, id);
        return;
    }

    ASSERT(wire_type >= 0 && wire_type < OF_TABLE_FEATURE_PROP_ITEM_COUNT);

    *id = of_table_feature_prop_type_to_id[obj->version][wire_type];
    ASSERT(*id != OF_OBJECT_INVALID);
}

/**
 * Get the object ID based on the wire buffer for meter_band object
 * @param obj The object being referenced
 * @param id Where to store the object ID
 */

void
of_meter_band_wire_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    int wire_type;

    of_tlv16_wire_type_get(obj, &wire_type);
    if (wire_type == OF_EXPERIMENTER_TYPE) {
        *id = OF_METER_BAND_EXPERIMENTER;
        return;
    }

    ASSERT(wire_type >= 0 && wire_type < OF_METER_BAND_ITEM_COUNT);

    *id = of_meter_band_type_to_id[obj->version][wire_type];
    ASSERT(*id != OF_OBJECT_INVALID);
}

/**
 * Get the object ID based on the wire buffer for a hello_elem object
 * @param obj The object being referenced
 * @param id Where to store the object ID
 */

void
of_hello_elem_wire_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    int wire_type;

    of_tlv16_wire_type_get(obj, &wire_type);
    ASSERT(wire_type >= 0 && wire_type < OF_HELLO_ELEM_ITEM_COUNT);
    *id = of_hello_elem_type_to_id[obj->version][wire_type];
    ASSERT(*id != OF_OBJECT_INVALID);
}

/****************************************************************
 * OXM type/length functions.
 ****************************************************************/

/* Where does the OXM type-length header lie in the buffer */
#define OXM_HDR_OFFSET 0

/**
 * Get the OXM header (type-length fields) from the wire buffer
 * associated with an OXM object
 *
 * Will return if error; set hdr to the OXM header
 */

#define _GET_OXM_TYPE_LEN(obj, tl_p, wbuf) do {                         \
        wbuf = OF_OBJECT_TO_WBUF(obj);                                  \
        ASSERT(wbuf != NULL);                                           \
        of_wire_buffer_u32_get(wbuf,                                    \
            OF_OBJECT_ABSOLUTE_OFFSET(obj, OXM_HDR_OFFSET), (tl_p));    \
    } while (0)

#define _SET_OXM_TYPE_LEN(obj, tl_p, wbuf) do {                         \
        wbuf = OF_OBJECT_TO_WBUF(obj);                                  \
        ASSERT(wbuf != NULL);                                           \
        of_wire_buffer_u32_set(wbuf,                                    \
            OF_OBJECT_ABSOLUTE_OFFSET(obj, OXM_HDR_OFFSET), (tl_p));    \
    } while (0)

/**
 * Get the length of an OXM object from the wire buffer
 * @param obj The object whose wire buffer is an OXM type
 * @param bytes (out) Where length is stored 
 */

void
of_oxm_wire_length_get(of_object_t *obj, int *bytes)
{
    uint32_t type_len;
    of_wire_buffer_t *wbuf;

    _GET_OXM_TYPE_LEN(obj, &type_len, wbuf);
    *bytes = OF_OXM_LENGTH_GET(type_len);
}

/**
 * Set the length of an OXM object in the wire buffer
 * @param obj The object whose wire buffer is an OXM type
 * @param bytes Value to store in wire buffer
 */

void
of_oxm_wire_length_set(of_object_t *obj, int bytes)
{
    uint32_t type_len;
    of_wire_buffer_t *wbuf;

    ASSERT(bytes >= 0 && bytes < 256);

    /* Read-modify-write */
    _GET_OXM_TYPE_LEN(obj, &type_len, wbuf);
    OF_OXM_LENGTH_SET(type_len, bytes);
    of_wire_buffer_u32_set(wbuf, 
           OF_OBJECT_ABSOLUTE_OFFSET(obj, OXM_HDR_OFFSET), type_len);
}

/**
 * Get the object ID of an OXM object based on the wire buffer type
 * @param obj The object whose wire buffer is an OXM type
 * @param id (out) Where the ID is stored 
 */

void
of_oxm_wire_object_id_get(of_object_t *obj, of_object_id_t *id)
{
    uint32_t type_len;
    int wire_type;
    of_wire_buffer_t *wbuf;

    _GET_OXM_TYPE_LEN(obj, &type_len, wbuf);
    wire_type = OF_OXM_MASKED_TYPE_GET(type_len);
    *id = of_oxm_to_object_id(wire_type, obj->version);
}

/**
 * Set the wire type of an OXM object based on the object ID passed
 * @param obj The object whose wire buffer is an OXM type
 * @param id The object ID mapped to an OXM wire type which is stored
 */

void
of_oxm_wire_object_id_set(of_object_t *obj, of_object_id_t id)
{
    uint32_t type_len;
    int wire_type;
    of_wire_buffer_t *wbuf;

    ASSERT(OF_OXM_VALID_ID(id));

    /* Read-modify-write */
    _GET_OXM_TYPE_LEN(obj, &type_len, wbuf);
    wire_type = of_object_to_wire_type(id, obj->version);
    ASSERT(wire_type >= 0);
    OF_OXM_MASKED_TYPE_SET(type_len, wire_type);
    of_wire_buffer_u32_set(wbuf, 
           OF_OBJECT_ABSOLUTE_OFFSET(obj, OXM_HDR_OFFSET), type_len);
}



#define OF_U16_LEN_LENGTH_OFFSET 0

/**
 * Get the wire length for an object with a uint16 length as first member
 * @param obj The object being referenced
 * @param bytes Pointer to location to store length
 */
void
of_u16_len_wire_length_get(of_object_t *obj, int *bytes)
{
    of_wire_buffer_t *wbuf = OF_OBJECT_TO_WBUF(obj);
    uint16_t u16;

    ASSERT(wbuf != NULL);

    of_wire_buffer_u16_get(wbuf, 
           OF_OBJECT_ABSOLUTE_OFFSET(obj, OF_U16_LEN_LENGTH_OFFSET),
           &u16);

    *bytes = u16;
}

/**
 * Set the wire length for an object with a uint16 length as first member
 * @param obj The object being referenced
 * @param bytes The length of the object
 */

void
of_u16_len_wire_length_set(of_object_t *obj, int bytes)
{
    of_wire_buffer_t *wbuf = OF_OBJECT_TO_WBUF(obj);
    ASSERT(wbuf != NULL);

    /* ASSERT(obj is u16-len entry) */

    of_wire_buffer_u16_set(wbuf, 
           OF_OBJECT_ABSOLUTE_OFFSET(obj, OF_U16_LEN_LENGTH_OFFSET),
           bytes);
}


#define OF_PACKET_QUEUE_LENGTH_OFFSET(ver) \
    (((ver) >= OF_VERSION_1_2) ? 8 : 4)

/**
 * Get the wire length for a packet queue object
 * @param obj The object being referenced
 * @param bytes Pointer to location to store length
 *
 * The length is a uint16 at the offset indicated above
 */
void
of_packet_queue_wire_length_get(of_object_t *obj, int *bytes)
{
    of_wire_buffer_t *wbuf = OF_OBJECT_TO_WBUF(obj);
    uint16_t u16;
    int offset;

    ASSERT(wbuf != NULL);

    /* ASSERT(obj is packet queue obj) */
    offset = OF_PACKET_QUEUE_LENGTH_OFFSET(obj->version);
    of_wire_buffer_u16_get(wbuf, OF_OBJECT_ABSOLUTE_OFFSET(obj, offset),
                           &u16);

    *bytes = u16;
}

/**
 * Set the wire length for a 1.2 packet queue object
 * @param obj The object being referenced
 * @param bytes The length of the object
 *
 * The length is a uint16 at the offset indicated above
 */

void
of_packet_queue_wire_length_set(of_object_t *obj, int bytes)
{
    of_wire_buffer_t *wbuf = OF_OBJECT_TO_WBUF(obj);
    int offset;

    ASSERT(wbuf != NULL);

    /* ASSERT(obj is packet queue obj) */
    offset = OF_PACKET_QUEUE_LENGTH_OFFSET(obj->version);
    of_wire_buffer_u16_set(wbuf, OF_OBJECT_ABSOLUTE_OFFSET(obj, offset),
                                  bytes);
}

/**
 * Get the wire length for a meter band stats list
 * @param obj The object being referenced
 * @param bytes Pointer to location to store length
 *
 * Must a meter_stats object as a parent
 */

void
of_list_meter_band_stats_wire_length_get(of_object_t *obj, int *bytes)
{
    ASSERT(obj->parent != NULL);
    ASSERT(obj->parent->object_id == OF_METER_STATS);

    /* We're counting on the parent being properly initialized already.
     * The length is stored in a uint16 at offset 4 of the parent.
     */
    *bytes = obj->parent->length - OF_OBJECT_FIXED_LENGTH(obj->parent);
}

#define OF_METER_STATS_LENGTH_OFFSET 4

/**
 * Get/set the wire length for a meter stats object
 * @param obj The object being referenced
 * @param bytes Pointer to location to store length
 *
 * It's almost a TLV....
 */

void
of_meter_stats_wire_length_get(of_object_t *obj, int *bytes)
{
    uint16_t val16;
    of_wire_buffer_t *wbuf = OF_OBJECT_TO_WBUF(obj);
    ASSERT(wbuf != NULL);
    of_wire_buffer_u16_get(wbuf, 
               OF_OBJECT_ABSOLUTE_OFFSET(obj, OF_METER_STATS_LENGTH_OFFSET),
               &val16);
    *bytes = val16;
}

void
of_meter_stats_wire_length_set(of_object_t *obj, int bytes)
{
    of_wire_buffer_t *wbuf = OF_OBJECT_TO_WBUF(obj);
    ASSERT(wbuf != NULL);

    of_wire_buffer_u16_set(wbuf, 
        OF_OBJECT_ABSOLUTE_OFFSET(obj, OF_METER_STATS_LENGTH_OFFSET), bytes);
}

/*
 * Non-message extension push wire values
 */

int
of_extension_object_wire_push(of_object_t *obj)
{
    of_action_bsn_mirror_t *action_mirror;
    of_action_id_bsn_mirror_t *action_id_mirror;
    of_action_bsn_set_tunnel_dst_t *action_set_tunnel_dst;
    of_action_id_bsn_set_tunnel_dst_t *action_id_set_tunnel_dst;
    of_action_nicira_dec_ttl_t *action_nicira_dec_ttl;
    of_action_id_nicira_dec_ttl_t *action_id_nicira_dec_ttl;

    /* Push exp type, subtype */
    switch (obj->object_id) {
    case OF_ACTION_BSN_MIRROR:
        action_mirror = (of_action_bsn_mirror_t *)obj;
        of_action_bsn_mirror_experimenter_set(action_mirror,
            OF_EXPERIMENTER_ID_BSN);
        of_action_bsn_mirror_subtype_set(action_mirror, 1);
        break;
    case OF_ACTION_ID_BSN_MIRROR:
        action_id_mirror = (of_action_id_bsn_mirror_t *)obj;
        of_action_id_bsn_mirror_experimenter_set(action_id_mirror,
            OF_EXPERIMENTER_ID_BSN);
        of_action_id_bsn_mirror_subtype_set(action_id_mirror, 1);
        break;
    case OF_ACTION_BSN_SET_TUNNEL_DST:
        action_set_tunnel_dst = (of_action_bsn_set_tunnel_dst_t *)obj;
        of_action_bsn_set_tunnel_dst_experimenter_set(action_set_tunnel_dst,
            OF_EXPERIMENTER_ID_BSN);
        of_action_bsn_set_tunnel_dst_subtype_set(action_set_tunnel_dst, 2);
        break;
    case OF_ACTION_ID_BSN_SET_TUNNEL_DST:
        action_id_set_tunnel_dst = (of_action_id_bsn_set_tunnel_dst_t *)obj;
        of_action_id_bsn_set_tunnel_dst_experimenter_set(action_id_set_tunnel_dst,
            OF_EXPERIMENTER_ID_BSN);
        of_action_id_bsn_set_tunnel_dst_subtype_set(action_id_set_tunnel_dst, 2);
        break;
    case OF_ACTION_NICIRA_DEC_TTL:
        action_nicira_dec_ttl = (of_action_nicira_dec_ttl_t *)obj;
        of_action_nicira_dec_ttl_experimenter_set(action_nicira_dec_ttl,
            OF_EXPERIMENTER_ID_NICIRA);
        of_action_nicira_dec_ttl_subtype_set(action_nicira_dec_ttl, 18);
        break;
    case OF_ACTION_ID_NICIRA_DEC_TTL:
        action_id_nicira_dec_ttl = (of_action_id_nicira_dec_ttl_t *)obj;
        of_action_id_nicira_dec_ttl_experimenter_set(action_id_nicira_dec_ttl,
            OF_EXPERIMENTER_ID_NICIRA);
        of_action_id_nicira_dec_ttl_subtype_set(action_id_nicira_dec_ttl, 18);
        break;
    default:
        break;
    }

    return OF_ERROR_NONE;
}
