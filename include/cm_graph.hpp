#ifndef __CYAN_MISCELIUM_GRAPH__
#define __CYAN_MISCELIUM_GRAPH__

#include "cm.h"
#include "concurrent/cm_concurrent.hpp"
#include "math/cm_tensor.hpp"
#include "memory/cm_memory_manager.hpp"
#include "nodes/cm_nodes_commons.hpp"
#include "collections/cm_collections.hpp"
#include "cm_engine_mem.hpp"

namespace CyanMycelium
{
    // forward declaration
    class Node;
    class Link;
    class UnaryOperator;
    class BinaryOperator;

    typedef void (*UnaryFunctionPtr)(Tensor *, Tensor *, UnaryOperator *);
    typedef void (*BinaryFunctionPtr)(Tensor *, Tensor *, Tensor *, BinaryOperator *);

    /// @brief
    class IActivationCtx
    {
    public:
        IMemoryManagerPtr MemoryManager;

        virtual bool Activate(Node *) = 0;
        virtual bool Activate(Link *, void * = nullptr) = 0;

    protected:
        IActivationCtx(IMemoryManagerPtr mm)
        {
            MemoryManager = mm;
        }
    };
    typedef IActivationCtx *IActivationCtxPtr;

    /// @brief
    class GraphItem
    {
    };
    typedef GraphItem *GraphItemPtr;

    /// @brief
    class Link : public GraphItem
    {
    public:
        Link() : Link(nullptr, 1){};
        Link(const uint32_t *shape, int dimension, tensor_data_type_t type = TDT_UNDEFINED) : Payload(shape, dimension, type)
        {
            Oini = nullptr;
            Ofin = nullptr;
        };
        Node *Oini;
        Node *Ofin;
        Tensor Payload;
        virtual bool Activate(IActivationCtxPtr ctx, void * = nullptr);
    };

    typedef Link *LinkPtr;

    /// @brief
    class LinkCollection : public Collection<LinkPtr>
    {
    public:
        LinkCollection(unsigned int initialCapacity = 2) : Collection<LinkPtr>(initialCapacity)
        {
        }
    };

    /// @brief
    class Node : public GraphItem
    {
    public:
        Node() : _lock()
        {
        }
        LinkCollection Opsc;
        LinkCollection Onsc;
        MutexPtr GetLock()
        {
            return &_lock;
        }
        virtual bool Activate(IActivationCtxPtr ctx) = 0;

    private:
        Mutex _lock;
    };

    typedef Node *NodePtr;

    /// @brief
    class NodeCollection : public Collection<NodePtr>
    {
    public:
        NodeCollection(unsigned int initialCapacity = 2) : Collection<NodePtr>(initialCapacity)
        {
        }
    };

    /// @brief
    class Operator : public Node
    {
    protected:
        bool ForwardOuput(TensorPtr output, IActivationCtxPtr ctx);
    };
    typedef Operator *OperatorPtr;

    /// @brief
    class UnaryOperator : public Operator
    {
    public:
        UnaryOperator(const UnaryFunctionPtr typedFn[TDT_COUNT]) : Operator() { _typedFn = typedFn; }
        bool Activate(IActivationCtxPtr ctx) override;

    protected:
        const UnaryFunctionPtr *_typedFn;
    };
    typedef UnaryOperator *UnaryOperatorPtr;

    /// @brief
    class BinaryOperator : public Operator
    {
    public:
        BinaryOperator(const BinaryFunctionPtr typedFn[TDT_COUNT]) : Operator() { this->_typedFn = typedFn; }
        bool Activate(IActivationCtxPtr ctx) override;

    protected:
        const BinaryFunctionPtr *_typedFn;
    };
    typedef BinaryOperator *BinaryOperatorPtr;

    /// @brief The Runtime support for the computational graph.
    /// As node we use  Object Positive Semi Conductor (Opsc) as input
    /// and Object Negativ Semi Conductor (Onsc) as output
    /// please not this is a Runtime support and all the necessay initialization has to be done by
    /// a corresponding GraphBuilder
    class Graph : public Node
    {
    public:
        /// @brief NULL terminated list of nodes.
        NodeCollection Nodes;
        /// @brief NULL terminated list of links.
        LinkCollection Links;

        KeyValueCollection<LinkPtr> inputs;
        KeyValueCollection<LinkPtr> outputs;

        bool Activate(IActivationCtxPtr ctx) override;
    };
    typedef Graph *GraphPtr;
}
#endif