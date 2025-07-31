#ifndef EDGE_FACADE_H
#define EDGE_FACADE_H

#include <memory>
#include <libxml/tree.h>
#include <QUuid>

class EdgeFacade {
public:
    struct Concept {
        virtual ~Concept() = default;
        virtual QUuid id() const = 0;
        virtual xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent) const = 0;
        virtual void read(xmlNodePtr node) = 0;
    };
    
    template<typename T>
    struct Model : public Concept {
        T* obj;
        explicit Model(T* o) : obj(o) {}
        
        QUuid id() const override { return obj->getId(); }
        xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent) const override {
            return obj->write(doc, parent);
        }
        void read(xmlNodePtr node) override { obj->read(node); }
    };
    
private:
    std::unique_ptr<Concept> m_impl;
    
public:
    template<typename T>
    explicit EdgeFacade(T* obj) : m_impl(std::make_unique<Model<T>>(obj)) {}
    
    EdgeFacade(const EdgeFacade&) = delete;
    EdgeFacade& operator=(const EdgeFacade&) = delete;
    EdgeFacade(EdgeFacade&&) = default;
    EdgeFacade& operator=(EdgeFacade&&) = default;
    
    QUuid id() const { return m_impl->id(); }
    xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent = nullptr) const {
        return m_impl->write(doc, parent);
    }
    void read(xmlNodePtr node) { m_impl->read(node); }
};

#endif // EDGE_FACADE_H