/*
Copyright 2024 Marvell Technology, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "pnaProgramStructure.h"

namespace P4::BMV2 {

using namespace P4::literals;

void InspectPnaProgram::postorder(const IR::Declaration_Instance *di) {
    if (!pinfo->resourceMap.count(di)) return;
    auto blk = pinfo->resourceMap.at(di);
    if (blk->is<IR::ExternBlock>()) {
        auto eb = blk->to<IR::ExternBlock>();
        LOG3("populate " << eb);
        pinfo->extern_instances.emplace(di->name, di);
    }
}

void InspectPnaProgram::addHeaderType(const IR::Type_StructLike *st) {
    LOG5("In addHeaderType with struct " << st->toString());
    if (st->is<IR::Type_HeaderUnion>()) {
        LOG5("Struct is Type_HeaderUnion");
        for (auto f : st->fields) {
            auto ftype = typeMap->getType(f, true);
            auto ht = ftype->to<IR::Type_Header>();
            CHECK_NULL(ht);
            addHeaderType(ht);
        }
        pinfo->header_union_types.emplace(st->getName(), st->to<IR::Type_HeaderUnion>());
        return;
    } else if (st->is<IR::Type_Header>()) {
        LOG5("Struct is Type_Header");
        pinfo->header_types.emplace(st->getName(), st->to<IR::Type_Header>());
    } else if (st->is<IR::Type_Struct>()) {
        LOG5("Struct is Type_Struct");
        pinfo->metadata_types.emplace(st->getName(), st->to<IR::Type_Struct>());
    }
}

void InspectPnaProgram::addHeaderInstance(const IR::Type_StructLike *st, cstring name) {
    auto inst = new IR::Declaration_Variable(name, st);
    if (st->is<IR::Type_Header>())
        pinfo->headers.emplace(name, inst);
    else if (st->is<IR::Type_Struct>())
        pinfo->metadata.emplace(name, inst);
    else if (st->is<IR::Type_HeaderUnion>())
        pinfo->header_unions.emplace(name, inst);
}

void InspectPnaProgram::addTypesAndInstances(const IR::Type_StructLike *type, bool isHeader) {
    LOG5("Adding type " << type->toString() << " and isHeader " << isHeader);
    for (auto f : type->fields) {
        LOG5("Iterating through field " << f->toString());
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            // The headers struct can not contain nested structures.
            if (isHeader && ft->is<IR::Type_Struct>()) {
                ::P4::error(ErrorType::ERR_INVALID,
                            "Type %1% should only contain headers, header stacks, or header unions",
                            type);
                return;
            }
            auto st = ft->to<IR::Type_StructLike>();
            addHeaderType(st);
        }
    }

    for (auto f : type->fields) {
        auto ft = typeMap->getType(f, true);
        if (ft->is<IR::Type_StructLike>()) {
            if (auto hft = ft->to<IR::Type_Header>()) {
                LOG5("Field is Type_Header");
                addHeaderInstance(hft, f->controlPlaneName());
            } else if (ft->is<IR::Type_HeaderUnion>()) {
                LOG5("Field is Type_HeaderUnion");
                for (auto uf : ft->to<IR::Type_HeaderUnion>()->fields) {
                    auto uft = typeMap->getType(uf, true);
                    if (auto h_type = uft->to<IR::Type_Header>()) {
                        addHeaderInstance(h_type, uf->controlPlaneName());
                    } else {
                        ::P4::error(ErrorType::ERR_INVALID, "Type %1% cannot contain type %2%", ft,
                                    uft);
                        return;
                    }
                }
                pinfo->header_union_types.emplace(type->getName(),
                                                  type->to<IR::Type_HeaderUnion>());
                addHeaderInstance(type, f->controlPlaneName());
            } else {
                LOG5("Adding struct with type " << type);
                pinfo->metadata_types.emplace(type->getName(), type->to<IR::Type_Struct>());
                addHeaderInstance(type, f->controlPlaneName());
            }
        } else if (ft->is<IR::Type_Stack>()) {
            LOG5("Field is Type_Stack " << ft->toString());
            auto stack = ft->to<IR::Type_Stack>();
            // auto stack_name = f->controlPlaneName();
            auto stack_size = stack->getSize();
            auto type = typeMap->getTypeType(stack->elementType, true);
            BUG_CHECK(type->is<IR::Type_Header>(), "%1% not a header type", stack->elementType);
            auto ht = type->to<IR::Type_Header>();
            addHeaderType(ht);
            auto stack_type = stack->elementType->to<IR::Type_Header>();
            std::vector<unsigned> ids;
            for (unsigned i = 0; i < stack_size; i++) {
                cstring hdrName = f->controlPlaneName() + "[" + Util::toString(i) + "]";
                /* TODO */
                // auto id = json->add_header(stack_type, hdrName);
                addHeaderInstance(stack_type, hdrName);
                // ids.push_back(id);
            }
            // addHeaderStackInstance();
        } else {
            // Treat this field like a scalar local variable
            cstring newName = refMap->newName(type->getName() + "." + f->name);
            LOG5("newname for scalarMetadataFields is " << newName);
            if (ft->is<IR::Type_Bits>()) {
                LOG5("Field is a Type_Bits");
                auto tb = ft->to<IR::Type_Bits>();
                pinfo->scalars_width += tb->size;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Boolean>()) {
                LOG5("Field is a Type_Boolean");
                pinfo->scalars_width += 1;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else if (ft->is<IR::Type_Error>()) {
                LOG5("Field is a Type_Error");
                pinfo->scalars_width += 32;
                pinfo->scalarMetadataFields.emplace(f, newName);
            } else {
                BUG("%1%: Unhandled type for %2%", ft, f);
            }
        }
    }
}

bool InspectPnaProgram::preorder(const IR::Declaration_Variable *dv) {
    auto ft = typeMap->getType(dv->getNode(), true);
    cstring scalarsName = refMap->newName("scalars");

    if (ft->is<IR::Type_Bits>()) {
        LOG5("Adding " << dv << " into scalars map");
        pinfo->scalars.emplace(scalarsName, dv);
    } else if (ft->is<IR::Type_Boolean>()) {
        LOG5("Adding " << dv << " into scalars map");
        pinfo->scalars.emplace(scalarsName, dv);
    }

    return false;
}

// This visitor only visits the parameter in the statement from architecture.
bool InspectPnaProgram::preorder(const IR::Parameter *param) {
    auto ft = typeMap->getType(param->getNode(), true);
    LOG3("add param " << ft);
    // only convert parameters that are IR::Type_StructLike
    if (!ft->is<IR::Type_StructLike>()) {
        return false;
    }
    auto st = ft->to<IR::Type_StructLike>();
    // check if it is pna specific standard metadata
    cstring ptName = param->type->toString();
    // parameter must be a type that we have not seen before
    if (pinfo->hasVisited(st)) {
        LOG5("Parameter is visited, returning");
        return false;
    }
    if (PnaProgramStructure::isStandardMetadata(ptName)) {
        LOG5("Parameter is pna standard metadata");
        addHeaderType(st);
        // remove _t from type name
        cstring headerName = ptName.exceptLast(2);
        addHeaderInstance(st, headerName);
    }
    auto isHeader = isHeaders(st);
    addTypesAndInstances(st, isHeader);
    return false;
}

void InspectPnaProgram::postorder(const IR::P4Parser *p) {
    if (pinfo->block_type.count(p)) {
        auto info = pinfo->block_type.at(p);
        if (info == MAIN_PARSER) pinfo->parsers.emplace("main_parser"_cs, p);
    }
}

void InspectPnaProgram::postorder(const IR::P4Control *c) {
    if (pinfo->block_type.count(c)) {
        auto info = pinfo->block_type.at(c);
        if (info == MAIN_CONTROL)
            pinfo->pipelines.emplace("main_control"_cs, c);
        else if (info == MAIN_DEPARSER)
            pinfo->deparsers.emplace("main_deparser"_cs, c);
    }
}

bool ParsePnaArchitecture::preorder(const IR::ExternBlock *block) {
    if (block->node->is<IR::Declaration>()) structure->globals.push_back(block);
    return false;
}

bool ParsePnaArchitecture::preorder(const IR::PackageBlock *block) {
    auto pkg = block->findParameterValue("main_parser"_cs);
    if (pkg == nullptr) {
        modelError("Package %1% has no parameter named 'main_parser'", block);
        return false;
    }
    auto main_parser = pkg->to<IR::ParserBlock>();
    if (main_parser == nullptr) {
        modelError("%1%: 'main_parser' argument should be in bound", block);
        return false;
    }

    pkg = block->findParameterValue("main_control"_cs);
    if (pkg == nullptr) {
        modelError("Package %1% has no parameter named 'main_control'", block);
        return false;
    }
    auto main_control = pkg->to<IR::ControlBlock>();
    if (main_control == nullptr) {
        modelError("%1%: 'main_control' argument should be in bound", block);
        return false;
    }

    pkg = block->findParameterValue("main_deparser"_cs);
    if (pkg == nullptr) {
        modelError("Package %1% has no parameter named 'main_deparser'", block);
        return false;
    }
    auto main_deparser = pkg->to<IR::ControlBlock>();
    if (main_deparser == nullptr) {
        modelError("%1%: 'main_deparser' argument should be in bound", block);
        return false;
    }
    structure->block_type.emplace(main_parser->container, MAIN_PARSER);
    structure->block_type.emplace(main_control->container, MAIN_CONTROL);
    structure->block_type.emplace(main_deparser->container, MAIN_DEPARSER);
    structure->pipeline_controls.emplace(main_control->container->name);
    structure->non_pipeline_controls.emplace(main_deparser->container->name);

    return false;
}

}  // namespace P4::BMV2
