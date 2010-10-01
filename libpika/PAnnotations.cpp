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
        String* doc_String = ctx->GetEngine()->AllocStringNC("__doc"); // TODO: This string will be used enough to add it as a root object.
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
        }
        else if (engine->Package_Type->IsInstance(obj))
        {
            Package* subj = static_cast<Package*> (obj);
            subj->SetSuper(pkg);
        }
        else {
            Type* type = obj->GetType();
            const char* type_str = type->GetName()->GetBuffer();
            RaiseException(Exception::ERROR_type, "attempt to set scope on object of type %s: expecting a Function or Package.", type_str);
        }
        ctx->Push(obj);
		return 1;
	}
	
	int Annotation_strict(Context* ctx, Value&)
	{
        Function* func = ctx->GetArgT<Function>(0);        
        Def* def = func->GetDef();
        if (def->isVarArg || def->isKeyword || func->defaults != 0)
            RaiseException("attempt to make function %s strict: functions with default values, variable arguments or keyword arguments cannot be strict.\n", func->GetName()->GetBuffer());
        else
            def->isStrict = true;
        ctx->Push(func);
		return 1;
	}
            
    /*
    Problems {
        + Returned value is a Context not a Function.
        + This could break other Function annotations.
        + Need to override opCall to it behaves like a function.
        + Context still needs to be setup. 
    }
    int Annotation_coro(Context* ctx, Value&)
    {
        Engine* eng = ctx->GetEngine();
        GCPAUSE(eng);
        Function* func = ctx->GetObjectArg(0);
        Context* coro = Context::Create(eng, eng->Context_Type);
        coro->PushNull();
        coro->Push(func);
        
        ctx->Push(coro);
    }
    */
}// namespace

void Init_Annotations(Engine* engine, Package* world)
{
	static RegisterFunction annotations[] = {
        {"abstract", Annotation_abstract, 1, DEF_STRICT },
        {"attr",     Annotation_attr,     1, DEF_KEYWORD_ARGS | DEF_STRICT },
        {"bindto",   Annotation_bindto,   2, DEF_STRICT },
        {"doc",      Annotation_doc,      2, DEF_STRICT },
        {"final",    Annotation_final,    1, DEF_STRICT },
        {"scope",    Annotation_scope,    2, DEF_STRICT },
        {"strict",   Annotation_strict,   1, DEF_STRICT },
    };
    
    world->EnterFunctions(annotations, countof(annotations));
}

}// pika
