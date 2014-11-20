/*
 *  PAnnotations.cpp
 *  See Copyright Notice in Pika.h
 */
#include "Pika.h"
#include "PObject.h"
#include "PContext.h"
#include "PString.h"
#include "PFunction.h"
#include "PType.h"

namespace pika {
namespace {
	/**
        getter => [function] creates property read-only
        setter => [function] creates property write-only
	  	close: [function] mustClose = true
	  	close 'full' ->  sets scope for all parent functions
		meta: [class] type.type = metatype
        bindto: =>BoundFunction
  	*/
	int Annotation_abstract(Context* ctx, Value&)
	{
        Type* type = ctx->GetArgT<Type>(0);
        type->SetAbstract(true);
        ctx->Push(type);
		return 1;
	}
    
    int Annotation_attr(Context* ctx, Value&)
    {
        Object* obj = ctx->GetObjectArg(0);
        Dictionary* dict = ctx->GetKeywordArgs();
        if (!dict)
            RaiseException("annotation attr called with no named parameters.\n");
        for (Table::Iterator iter = dict->Elements().GetIterator(); iter; ++iter)
        {
            obj->SetSlot(iter->key, iter->val);
        }
        ctx->Push(obj);
        return 1;
    }
    
    int Annotation_bindto(Context* ctx, Value&)
    {
        Engine* eng = ctx->GetEngine();
        GCPAUSE(eng);
        Function* func = ctx->GetArgT<Function>(0);
        Value& subj = ctx->GetArg(1);
        
        BoundFunction* bound_func = BoundFunction::Create(eng, eng->BoundFunction_Type, func, subj);
        ctx->Push(bound_func);
        return 1;
    }
    
	int Annotation_doc(Context* ctx, Value&)
	{
        String* doc_String = ctx->GetEngine()->GetString("__doc"); // TODO: This string will be used enough to add it as a root object.
        Object* object = ctx->GetObjectArg(0);
        String* doc = ctx->GetStringArg(1);

        ctx->Push(doc);
        ctx->Push(object);        
        ctx->Push(doc_String);
        
        ctx->OpDotSet(OP_dotset, OVR_set);
        ctx->Push(object);
		return 1;
	}
	
	int Annotation_final(Context* ctx, Value&)
	{
        Type* type = ctx->GetArgT<Type>(0);
        type->SetFinal(true);
        ctx->Push(type);
		return 1;
	}
    
	int Annotation_scope(Context* ctx, Value&)
	{
        Engine* engine = ctx->GetEngine();
        Object* obj = ctx->GetObjectArg(0);
        Package* pkg = ctx->GetArgT<Package>(1);
        if (engine->Function_Type->IsInstance(obj))
        {
            Function* func = static_cast<Function*> (obj);
            func->location = pkg;
            func->WriteBarrier(pkg);
        }
        else if (engine->Package_Type->IsInstance(obj))
        {
            Package* subj = static_cast<Package*> (obj);
            subj->SetSuper(pkg);
        }
        else {
            Type* type = obj->GetType();
            const char* type_str = type->GetName()->GetBuffer();
            RaiseException(Exception::ERROR_type, "Attempt to set scope on object of type %s: expecting a Function or Package.", type_str);
        }
        ctx->Push(obj);
		return 1;
	}
        
    Package* FunctionGetPackage(Engine* eng, Function* f)
    {
        Package* pkg = f->location;
        if (eng->InstanceMethod_Type->IsInstance(f))
        {
            pkg = ((InstanceMethod*)f)->GetClassType();
        } 
        else if (eng->ClassMethod_Type->IsInstance(f))
        {
            pkg = ((ClassMethod*)f)->GetClassType();
        }
        if (!pkg)
            RaiseException("Cannot find the location of function '%s' needed by the annotation.", f->def->name->GetBuffer());
        return pkg;
    }
    
    Property* LookupProperty(Engine* eng, Function* f, String* name, Package* pkg)
    {        
        if (pkg)
        {
            Value p(NULL_VALUE);
            if (pkg->GetGlobal(name, p) && eng->Property_Type->IsInstance(p))
            {
                return p.val.property;
            }
        }
        return 0;
    }
    
    int Annotation_getter(Context* ctx, Value&)
    {
        GCPAUSE(ctx->GetEngine());
        Function* func = ctx->GetArgT<Function>(0);
        String* name = ctx->GetStringArg(1);
        Property* p = 0;
        Package* pkg = FunctionGetPackage(ctx->GetEngine(), func);
                
        if (p = LookupProperty(ctx->GetEngine(), func, name, pkg))
        {
            p->SetRead(func);
        }
        else
        {
            p = Property::CreateRead(ctx->GetEngine(), name, func);
            pkg->AddProperty(p);
        }
        ctx->Push(func);
        return 1;
    }
    
    int Annotation_setter(Context* ctx, Value&)
    {
        GCPAUSE(ctx->GetEngine());
        Function* func = ctx->GetArgT<Function>(0);
        String* name = ctx->GetStringArg(1);
        Property* p = 0;
        Package* pkg = FunctionGetPackage(ctx->GetEngine(), func);
        if (p = LookupProperty(ctx->GetEngine(), func, name, pkg))
        {
            p->SetWriter(func);
        }
        else
        {
            p = Property::CreateWrite(ctx->GetEngine(), name, func);
            pkg->AddProperty(p);
        }
        ctx->Push(func);
        return 1;
    }
    
}// namespace

void Init_Annotations(Engine* engine, Package* world)
{
	static RegisterFunction annotations[] = {
        {"abstract", Annotation_abstract, 1, DEF_STRICT, 0 },
        {"attr",     Annotation_attr,     1, DEF_KEYWORD_ARGS | DEF_STRICT, 0 },
        {"bindto",   Annotation_bindto,   2, DEF_STRICT, 0 },
        {"doc",      Annotation_doc,      2, DEF_STRICT, 0 },
        {"final",    Annotation_final,    1, DEF_STRICT, 0 },
        {"scope",    Annotation_scope,    2, DEF_STRICT, 0 },
        {"getter",   Annotation_getter,   2, DEF_STRICT, 0 },
        {"setter",   Annotation_setter,   2, DEF_STRICT, 0 },
    };
    
    world->EnterFunctions(annotations, countof(annotations));
}

}// pika
