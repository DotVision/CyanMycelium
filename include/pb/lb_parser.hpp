#ifndef _BLUESTEEL_LADYBUG_PULLPARSER__
#define _BLUESTEEL_LADYBUG_PULLPARSER__

#include "lb.h"
#include "lb_stream.hpp"

namespace BlueSteelLadyBug
{
#ifndef LB_MAX_READER_SNAPSHOT_DEPTH
#define LB_MAX_READER_SNAPSHOT_DEPTH 8
#endif

    enum WireType : lb_byte_t
    {
        PB_VARINT = 0,
        PB_64BIT = 1,
        PB_LEN = 2,
        PB_32BIT = 5
    };

    struct ReaderStatus
    {
        lb_uint32_t fieldNumber;
        WireType wireType;
        lb_uint16_t depth;
        lb_uint64_t length;
        bool lengthReaded;
    };

    struct ReaderSnapshot
    {
        size_t position;
        ReaderStatus status;
    };

    class PBReader
    {
    public:
        PBReader()
        {
            _status.wireType = PB_VARINT;
            _status.fieldNumber = 0;
            _status.fieldNumber = 0;
            _status.depth = 0;
            _status.length = 0;
            _status.lengthReaded = false;
            _activSnapshot = -1;
        }

        PBReader(IInputStream *input) : PBReader()
        {
            _input = input;
        }

        ~PBReader()
        {
        }

        /// @brief reads the next protobuf tag from the input source.
        /// @return true if the token was read successfully; otherwise, false.
        bool readTag();

        bool readLength(lb_uint64_t *, bool = true);

        bool readValue(lb_int32_t *v) { return _readValue(v, _status.wireType); }
        bool readValue(lb_int64_t *v) { return _readValue(v, _status.wireType); }
        bool readValue(lb_uint32_t *v) { return _readValue(v, _status.wireType); }
        bool readValue(lb_uint64_t *v) { return _readValue(v, _status.wireType); }
        bool readValue(lb_float_t *v) { return _readValue(v, _status.wireType); }
        bool readValue(lb_double_t *v) { return _readValue(v, _status.wireType); }
        bool readValue(lb_bool_t *);
        bool readValue(char *);
        bool readValue(lb_byte_t *);
        bool readValue_s(char *, int);
        bool readValue_s(lb_byte_t *, int);

        bool readPacked(lb_int32_t *v, WireType wt);
        bool readPacked(lb_int64_t *v, WireType wt);
        bool readPacked(lb_uint32_t *v, WireType wt);
        bool readPacked(lb_uint64_t *v, WireType wt);
        bool readPacked(lb_float_t *v);
        bool readPacked(lb_double_t *v);

        PBReader *getSubMessageReader();

        bool skip();

        /// @brief Save the current status of the parser. To do so, the underlying stream MUST support Seek operation.
        void save();

        /// @brief Restore the status previously saved. To do so, the underlying stream MUST support Seek operation
        void restore();

        /// @brief Clear the last save without restoring.
        void unsave();

        lb_uint32_t getFieldNumber() { return _status.fieldNumber; }
        WireType getWireType() { return _status.wireType; }
        lb_byte_t getDepth() { return _status.depth; }
        IInputStream *getInput() { return _input; }
        size_t getPosition() { return _input->getPosition(); }
        size_t getSize() { return _input->getSize(); }
        size_t getRemainingBytes() { return _input->getRemainingBytes(); }

    protected:
        ReaderStatus _status;
        IInputStream *_input;
        ReaderSnapshot _snapshots[LB_MAX_READER_SNAPSHOT_DEPTH];
        int _activSnapshot;

        bool _readVarint(lb_uint64_t *dest);
        bool _readSVarint(lb_int64_t *dest);
        bool _readFixed32(void *dest);
        bool _readFixed64(void *dest);

        bool _readValue(lb_int32_t *, WireType);
        bool _readValue(lb_int64_t *, WireType);
        bool _readValue(lb_uint32_t *, WireType);
        bool _readValue(lb_uint64_t *, WireType);
        bool _readValue(lb_float_t *, WireType);
        bool _readValue(lb_double_t *, WireType);

        template <typename T>
        bool _readPacked(T *, WireType);

        void _invalidateLengthReaded() { _status.lengthReaded = false; }
    };

    class PBSubReader : public PBReader
    {
    public:
        PBSubReader(PBReader *r, lb_uint16_t depth, size_t from, size_t length) : _sv(r->getInput(), from, length)
        {
            _status.depth = depth;
            _input = &(this->_sv);
            _parent = r;
        }

    private:
        PBReader *_parent;
        StreamView _sv;
    };
}

#endif