#ifndef GRAPH_FACADES_H
#define GRAPH_FACADES_H

#include <memory>
#include <libxml/tree.h>
#include <QUuid>
#include <QPointF>

//=============================================================================
// NodeFacade - Type-erasure for anything that behaves like a Node
//=============================================================================
class NodeFacade {
public:
    // Pure virtual interface for Node-like objects
    struct Concept {
        virtual ~Concept() = default;
        virtual QUuid id() const = 0;
        virtual QPointF position() const = 0;
        virtual void setPosition(const QPointF& pos) = 0;
        virtual QString nodeType() const = 0;
        virtual xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent) const = 0;
        virtual void read(xmlNodePtr node) = 0;
    };
    
    // Template wrapper for any Node-like class T
    template<typename T>
    struct Model : public Concept {
        T* obj;
        
        explicit Model(T* o) : obj(o) {}
        
        QUuid id() const override {
            return obj->getId();
        }
        
        QPointF position() const override {
            return obj->pos();
        }
        
        void setPosition(const QPointF& pos) override {
            obj->setPos(pos);
        }
        
        QString nodeType() const override {
            return obj->getNodeType();
        }
        
        xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent) const override {
            return obj->write(doc, parent);
        }
        
        void read(xmlNodePtr node) override {
            obj->read(node);
        }
    };
    
private:
    std::unique_ptr<Concept> m_impl;
    
public:
    // Constructor: wrap any Node-like object
    template<typename T>
    explicit NodeFacade(T* obj) : m_impl(std::make_unique<Model<T>>(obj)) {}
    
    // Move semantics only (no copying for now)
    NodeFacade(const NodeFacade&) = delete;
    NodeFacade& operator=(const NodeFacade&) = delete;
    NodeFacade(NodeFacade&&) = default;
    NodeFacade& operator=(NodeFacade&&) = default;
    
    // Uniform interface
    QUuid id() const { return m_impl->id(); }
    QPointF position() const { return m_impl->position(); }
    void setPosition(const QPointF& pos) { m_impl->setPosition(pos); }
    QString nodeType() const { return m_impl->nodeType(); }
    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent = nullptr) const {
        return m_impl->write(doc, parent);
    }
    void read(xmlNodePtr node) { m_impl->read(node); }
};

//=============================================================================
// EdgeFacade - Type-erasure for anything that behaves like an Edge
//=============================================================================
class EdgeFacade {
public:
    // Pure virtual interface for Edge-like objects
    struct Concept {
        virtual ~Concept() = default;
        virtual QUuid id() const = 0;
        virtual QUuid sourceNodeId() const = 0;
        virtual QUuid targetNodeId() const = 0;
        virtual int sourceSocketIndex() const = 0;
        virtual int targetSocketIndex() const = 0;
        virtual xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent) const = 0;
        virtual void read(xmlNodePtr node) = 0;
    };
    
    // Template wrapper for any Edge-like class T
    template<typename T>
    struct Model : public Concept {
        T* obj;
        
        explicit Model(T* o) : obj(o) {}
        
        QUuid id() const override {
            return obj->getId();
        }
        
        QUuid sourceNodeId() const override {
            return obj->fromNodeId();
        }
        
        QUuid targetNodeId() const override {
            return obj->toNodeId();
        }
        
        int sourceSocketIndex() const override {
            return obj->fromSocketIndex();
        }
        
        int targetSocketIndex() const override {
            return obj->toSocketIndex();
        }
        
        xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent) const override {
            return obj->write(doc, parent);
        }
        
        void read(xmlNodePtr node) override {
            obj->read(node);
        }
    };
    
private:
    std::unique_ptr<Concept> m_impl;
    
public:
    // Constructor: wrap any Edge-like object
    template<typename T>
    explicit EdgeFacade(T* obj) : m_impl(std::make_unique<Model<T>>(obj)) {}
    
    // Move semantics only
    EdgeFacade(const EdgeFacade&) = delete;
    EdgeFacade& operator=(const EdgeFacade&) = delete;
    EdgeFacade(EdgeFacade&&) = default;
    EdgeFacade& operator=(EdgeFacade&&) = default;
    
    // Uniform interface
    QUuid id() const { return m_impl->id(); }
    QUuid sourceNodeId() const { return m_impl->sourceNodeId(); }
    QUuid targetNodeId() const { return m_impl->targetNodeId(); }
    int sourceSocketIndex() const { return m_impl->sourceSocketIndex(); }
    int targetSocketIndex() const { return m_impl->targetSocketIndex(); }
    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent = nullptr) const {
        return m_impl->write(doc, parent);
    }
    void read(xmlNodePtr node) { m_impl->read(node); }
};

//=============================================================================
// Helper functions to create facades
//=============================================================================
template<typename T>
NodeFacade makeNodeFacade(T* obj) {
    return NodeFacade(obj);
}

template<typename T>
EdgeFacade makeEdgeFacade(T* obj) {
    return EdgeFacade(obj);
}

#endif // GRAPH_FACADES_H