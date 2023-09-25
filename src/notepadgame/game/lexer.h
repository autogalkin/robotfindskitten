#include "Scintilla.h"
#include "SciLexer.h"
// Lexilla.h should not be included here as it declares statically linked functions without the __declspec( dllexport )

// should inludes alls
// clang-format off
#include "WordList.h"
#include "PropSetSimple.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "LexerBase.h"
// clang-format on


class lexer : public Scintilla::ILexer5 {

    virtual int SCI_METHOD Version() const override { return 100; };
    virtual void SCI_METHOD Release() override{};
    virtual const char* SCI_METHOD PropertyNames() override { return ""; };
    virtual int SCI_METHOD PropertyType(const char* name) override {
        return SC_TYPE_BOOLEAN;
    };
    virtual const char* SCI_METHOD DescribeProperty(const char* name) override {
        return "";
    };
    virtual Sci_Position SCI_METHOD PropertySet(const char* key,
                                                const char* val) override {
        // if(props.Set(key, val)) { return 0; } else { return -1;
        return 0;
    };
    virtual const char* SCI_METHOD DescribeWordListSets() override {
        return "";
    };
    virtual Sci_Position SCI_METHOD WordListSet(int n,
                                                const char* wl) override {
        // if (n <numWordLists) { if (keyWordLists[n]->Set(wl)) { return 0;
        return 0;
    };
    virtual void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc,
                                int initStyle,
                                Scintilla::IDocument* pAccess) override {
      
                    return;
    };
    virtual void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position lengthDoc,
                                 int initStyle,
                                 Scintilla::IDocument* pAccess) override {
                    // not supported
    }
    virtual void* SCI_METHOD PrivateCall(int operation,
                                         void* pointer) override {
                    return NULL;
    }
    virtual int SCI_METHOD LineEndTypesSupported() override {
                    return SC_LINE_END_TYPE_DEFAULT;
    }
    virtual int SCI_METHOD AllocateSubStyles(int styleBase,
                                             int numberStyles) override {
                    return -1;
    }
    virtual int SCI_METHOD SubStylesStart(int styleBase) override {
                    return -1; }
    virtual int SCI_METHOD SubStylesLength(int styleBase) override {
                    return 0; }
    virtual int SCI_METHOD StyleFromSubStyle(int subStyle) override {
                    return subStyle;
    }
    virtual int SCI_METHOD PrimaryStyleFromStyle(int style) override {
                    return style;
    }
    virtual void SCI_METHOD FreeSubStyles() override {}
    virtual void SCI_METHOD SetIdentifiers(int style,
                                           const char* identifiers) override {
                    return;
    }
    virtual int SCI_METHOD DistanceToSecondaryStyles() override {
                    return 0; }
    virtual const char* SCI_METHOD GetSubStyleBases() override {
                    return ""; }
    virtual int SCI_METHOD NamedStyles() override {
                    return 0; }
    virtual const char* SCI_METHOD NameOfStyle(int style) override {
                    return "";
    }
    virtual const char* SCI_METHOD TagsOfStyle(int style) override {
                    return "";
    }
    virtual const char* SCI_METHOD DescriptionOfStyle(int style) override {
                    return "";
    }

    // ILexer5 methods

    virtual const char* SCI_METHOD GetName() override {
                    return "robotfindskitten";
    }
    virtual int SCI_METHOD GetIdentifier() override {
                    return 21; }
    virtual const char* SCI_METHOD PropertyGet(const char* key) override {
                    // props.Get(key);
                    return NULL;
    };
            };
