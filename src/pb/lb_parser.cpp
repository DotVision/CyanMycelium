#include "pb/lb_parser.hpp"

using namespace BlueSteelLadyBug;

template <typename T>
bool PBReader::_readPacked(T *v, WireType wt)
{
    if (_status.wireType != PB_LEN)
    {
        return false;
    }
    lb_uint64_t size;
    if (!readLength(&size))
    {
        return false;
    }
    _invalidateLengthReaded();
    size_t pos = getPosition();
    size_t end = pos + size;
    if (pos < end)
    {
        do
        {
            if (!_readValue(v, wt))
            {
                return false;
            }
            v++;
        } while (getPosition() < end);
    }
    return true;
}

bool PBReader::readLength(lb_uint64_t *v, bool validate)
{
    switch (_status.wireType)
    {
    case PB_LEN:
    {
        if (_status.lengthReaded)
        {
            *v = _status.length;
        }
        else
        {
            if (!_readVarint(v))
            {
                return false;
            }
            _status.length = *v;
            _status.lengthReaded = validate;
        }
        return true;
    }
    default:
    {
        return false;
    }
    }
}

bool PBReader::readTag()
{
    // read a tag.
    lb_uint64_t tag;
    if (_readVarint(&tag))
    {
        _status.fieldNumber = tag >> 3;
        _status.wireType = (WireType)(tag & 0x03);
        return true;
    }
    return false;
}

void PBReader::save()
{
    if (_input->canSeek() && _activSnapshot < (LB_MAX_READER_SNAPSHOT_DEPTH - 1))
    {
        _activSnapshot++;
        _snapshots[_activSnapshot].position = getPosition();
        _snapshots[_activSnapshot].status = _status;
    }
}

void PBReader::restore()
{
    if (_input->canSeek() && _activSnapshot >= 0)
    {
        _status = _snapshots[_activSnapshot].status;
        _input->seek(_snapshots[_activSnapshot].position, BEGIN);
        _activSnapshot--;
    }
}

void PBReader::unsave()
{
    if (_input->canSeek() && _activSnapshot >= 0)
    {
        _activSnapshot--;
    }
}

PBReader *PBReader::getSubMessageReader()
{
    lb_uint64_t l;
    if (!readLength(&l))
    {
        return nullptr;
    }
    _invalidateLengthReaded();

    return new PBSubReader(this, _status.depth + 1, getPosition(), l);
}

bool PBReader::_readVarint(lb_uint64_t *dest)
{
    lb_byte_t byte;

    if (_input->read(&byte) < 0)
    {
        return false;
    }

    if (byte & 0x80)
    {
        lb_uint64_t res = byte & 0x7F;
        lb_fastbyte_t shift = 0;
        do
        {
            shift += 7;
            if (_input->read(&byte) < 0)
            {
                return false;
            }
            res |= (lb_uint64_t)(byte & 0x7F) << shift;
        } while (byte & 0x80);

        if (dest)
        {
            *dest = res;
        }
        return true;
    }

    // short track for 1 byte value
    if (dest)
    {
        *dest = byte;
    }
    return true;
}

bool PBReader::_readSVarint(lb_int64_t *dest)
{

    lb_uint64_t value;
    bool r = _readVarint(&value);
    if (r)
    {
        *dest = (value & 1) ? (lb_int64_t)(~(value >> 1)) : (lb_int64_t)(value >> 1);
    }
    return r;
}

bool PBReader::_readFixed32(void *dest)
{
    union
    {
        lb_uint32_t fixed32;
        lb_byte_t bytes[4];
    } u;

    bool r = _input->read(u.bytes, 4);
    if (r)
    {
#if defined(LB_LITTLE_ENDIAN) && LB_LITTLE_ENDIAN == 1
        // fast track
        *(lb_uint32_t *)dest = u.fixed32;
#else
        *(lb_uint32_t *)dest = ((lb_uint32_t)u.bytes[0] << 0) |
                               ((lb_uint32_t)u.bytes[1] << 8) |
                               ((lb_uint32_t)u.bytes[2] << 16) |
                               ((lb_uint32_t)u.bytes[3] << 24);
#endif
    }
    return r;
}

bool PBReader::_readFixed64(void *dest)
{
    union
    {
        lb_uint64_t fixed64;
        lb_byte_t bytes[8];
    } u;

    bool r = _input->read(u.bytes, 8);
    if (r)
    {
#if defined(LB_LITTLE_ENDIAN) && LB_LITTLE_ENDIAN == 1
        // fast track
        *(lb_uint64_t *)dest = u.fixed64;
#else
        *(lb_uint64_t *)dest = ((lb_uint64_t)u.bytes[0] << 0) |
                               ((lb_uint64_t)u.bytes[1] << 8) |
                               ((lb_uint64_t)u.bytes[2] << 16) |
                               ((lb_uint64_t)u.bytes[3] << 24) |
                               ((lb_uint64_t)u.bytes[4] << 32) |
                               ((lb_uint64_t)u.bytes[5] << 40) |
                               ((lb_uint64_t)u.bytes[6] << 48) |
                               ((lb_uint64_t)u.bytes[7] << 56);
#endif
    }
    return r;
}

bool PBReader::_readValue(lb_int32_t *v, WireType wt)
{
    switch (wt)
    {
    case PB_VARINT:
    {
        lb_int64_t value;
        if (!_readSVarint(&value))
        {
            return false;
        }
        *v = (lb_int32_t)value;
        return true;
    }
    case PB_32BIT:
    {
        return _readFixed32(v);
    }
    default:
    {
        return false;
    }
    }
}

bool PBReader::_readValue(lb_int64_t *v, WireType wt)
{
    switch (wt)
    {
    case PB_VARINT:
    {
        return _readSVarint(v);
    }
    case PB_64BIT:
    {
        return _readFixed64(v);
    }
    default:
    {
        return false;
    }
    }
    return false;
}

bool PBReader::_readValue(lb_uint32_t *v, WireType wt)
{
    switch (wt)
    {
    case PB_VARINT:
    {
        lb_uint64_t value;
        if (!_readVarint(&value))
        {
            return false;
        }
        *v = (lb_uint32_t)value;

        return true;
    }
    case PB_32BIT:
    {
        return _readFixed32(v);
    }
    default:
    {
        return false;
    }
    }
    return false;
}

bool PBReader::_readValue(lb_uint64_t *v, WireType wt)
{
    switch (wt)
    {
    case PB_VARINT:
    {
        return _readVarint(v);
    }
    case PB_64BIT:
    {
        return _readFixed64(v);
    }
    default:
    {
        return false;
    }
    }
}

bool PBReader::_readValue(lb_float_t *v, WireType wt)
{
    switch (wt)
    {
    case (PB_32BIT):
    {
        lb_uint32_t value;
        if (!_readFixed32(&value))
        {
            return false;
        }
        *v = (lb_float_t)value;
        return true;
    }

    default:
    {
        return false;
    }
    }
}

bool PBReader::_readValue(lb_double_t *v, WireType wt)
{
    switch (wt)
    {
    case (PB_64BIT):
    {
        lb_uint64_t value;
        if (!_readFixed64(&value))
        {
            return false;
        }
        *v = (lb_double_t)value;
        return true;
    }

    default:
    {
        return false;
    }
    }
}

bool PBReader::readValue(lb_bool_t *v)
{
    lb_uint64_t value;
    if (!_readVarint(&value))
    {
        return false;
    }
    *v = (value != 0);
    return true;
}

bool PBReader::readValue(char *v)
{
    lb_uint64_t size;
    if (!readLength(&size))
    {
        return false;
    }
    _invalidateLengthReaded();
    lb_byte_t *t = (lb_byte_t *)v;
    int readed = _input->read(t, (int)size);
    if (readed != (int)size)
    {
        return false;
    };
    t[size] = 0; // null terminated
    return true;
}

bool PBReader::readValue(lb_byte_t *v)
{
    lb_uint64_t size;
    if (!readLength(&size))
    {
        return false;
    }
    _invalidateLengthReaded();
    return _input->read(v, (int)size) == (int)size;
}

bool PBReader::skip()
{
    lb_uint64_t size;

    switch (_status.wireType)
    {
    case PB_LEN:
    {
        if (!readLength(&size))
        {
            return false;
        }
        _invalidateLengthReaded();
        break;
    }
    case PB_VARINT:
    {
        return _readVarint(nullptr); // passing nullptr mean skip
    }
    case PB_32BIT:
    {
        size = 4;
        break;
    }
    case PB_64BIT:
    {
        size = 8;
        break;
    }
    }
    if (!_input->seek((int)size, CURRENT))
    {
        return false;
    }
    return true;
}

bool PBReader::readValue_s(char *v, int s)
{
    lb_uint64_t size;
    if (!readLength(&size))
    {
        return false;
    }
    _invalidateLengthReaded();
    lb_byte_t *t = (lb_byte_t *)v;
    int max_size = min((int)size, s - 1);
    if (_input->read(t, (int)max_size) != max_size)
    {
        return false;
    }
    if (max_size != (int)size && !_input->seek(size - max_size, CURRENT))
    {
        return false;
    }
    t[max_size] = 0; // null terminated
    return true;
}

bool PBReader::readValue_s(lb_byte_t *v, int s)
{
    lb_uint64_t size;
    if (!readLength(&size))
    {
        return false;
    }
    _invalidateLengthReaded();
    int max_size = min((int)size, s);
    if (_input->read(v, max_size) != max_size)
    {
        return false;
    }
    if (max_size != (int)size)
    {
        if (!_input->seek(size - max_size, CURRENT))
        {
            return false;
        }
    }
    return true;
}

bool PBReader::readPacked(lb_int32_t *v, WireType wt)
{
    return _readPacked<lb_int32_t>(v, wt);
}

bool PBReader::readPacked(lb_int64_t *v, WireType wt)
{
    return _readPacked<lb_int64_t>(v, wt);
}

bool PBReader::readPacked(lb_uint32_t *v, WireType wt)
{
    return _readPacked<lb_uint32_t>(v, wt);
}

bool PBReader::readPacked(lb_uint64_t *v, WireType wt)
{
    return _readPacked<lb_uint64_t>(v, wt);
}

bool PBReader::readPacked(lb_float_t *v)
{
    return _readPacked<lb_float_t>(v, PB_32BIT);
}

bool PBReader::readPacked(lb_double_t *v)
{
    return _readPacked<lb_double_t>(v, PB_64BIT);
}
