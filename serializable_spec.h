#ifndef SERIALIZABLE_SPEC_H
#define SERIALIZABLE_SPEC_H

#include "rubber_types/rubber_types.hpp"
#include <libxml/tree.h>
#include <QUuid>

// Minimal SerializableSpec that delegates to existing Node::write/read
struct SerializableSpec {
    struct Concept {
        virtual ~Concept() = default;
        virtual QUuid id() const = 0;
        virtual xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent) const = 0;
        virtual void read(xmlNodePtr node) = 0;
    };

    template<class Holder>
    struct Model : Holder, virtual Concept {
        using Holder::Holder;
        
        QUuid id() const override {
            return rubber_types::model_get(this).getId();
        }
        
        xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent) const override {
            // Delegate directly to the wrapped object's write method
            return rubber_types::model_get(this).write(doc, parent);
        }
        
        void read(xmlNodePtr node) override {
            // Delegate directly to the wrapped object's read method
            rubber_types::model_get(this).read(node);
        }
    };

    template<class Container>
    struct ExternalInterface : Container {
        using Container::Container;
        
        QUuid id() const {
            return rubber_types::interface_get(this).id();
        }
        
        xmlNodePtr write(xmlDocPtr doc, xmlNodePtr parent = nullptr) const {
            return rubber_types::interface_get(this).write(doc, parent);
        }
        
        void read(xmlNodePtr node) {
            rubber_types::interface_get(this).read(node);
        }
    };
};

// Generate the type-erased facade
using SerializableFacade = rubber_types::TypeErasure<SerializableSpec>;

#endif // SERIALIZABLE_SPEC_H