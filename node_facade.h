#ifndef NODE_FACADE_H
#define NODE_FACADE_H

#include <memory>
#include <libxml/tree.h>
#include <QUuid>
#include <QPointF>
#include <QString>

class NodeFacade {
public:
    struct Concept {
        virtual ~Concept() = default;
        virtual QUuid id() const = 0;
        virtual QPointF position() const = 0;
        virtual void setPosition(const QPointF& pos) = 0;
        virtual QString nodeType() const = 0;
        virtual xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent) const = 0;
        virtual void read(xmlNodePtr node) = 0;
    };
    
    template<typename T>
    struct Model : public Concept {
        T* obj;
        explicit Model(T* o) : obj(o) {}
        
        QUuid id() const override { return obj->getId(); }
        QPointF position() const override { return obj->pos(); }
        void setPosition(const QPointF& pos) override { obj->setPos(pos); }
        QString nodeType() const override { return obj->getNodeType(); }
        xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent) const override {
            return obj->write(doc, parent);
        }
        void read(xmlNodePtr node) override { obj->read(node); }
    };
    
private:
    std::unique_ptr<Concept> m_impl;
    
public:
    template<typename T>
    explicit NodeFacade(T* obj) : m_impl(std::make_unique<Model<T>>(obj)) {}
    
    NodeFacade(const NodeFacade&) = delete;
    NodeFacade& operator=(const NodeFacade&) = delete;
    NodeFacade(NodeFacade&&) = default;
    NodeFacade& operator=(NodeFacade&&) = default;
    
    QUuid id() const { return m_impl->id(); }
    QPointF position() const { return m_impl->position(); }
    void setPosition(const QPointF& pos) { m_impl->setPosition(pos); }
    QString nodeType() const { return m_impl->nodeType(); }
    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent = nullptr) const {
        return m_impl->write(doc, parent);
    }
    void read(xmlNodePtr node) { m_impl->read(node); }
};

#endif // NODE_FACADE_H