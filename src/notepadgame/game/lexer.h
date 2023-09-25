#include "Windows.h"
#include <iostream>
// should inludes alls in this order
// clang-format off
#include <string>
#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"
// Lexilla.h should not be included here as it declares statically linked functions without the __declspec( dllexport )
#include "WordList.h"
#include "PropSetSimple.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "LexerBase.h"
#include "DefaultLexer.h"
// clang-format on
//
//
static const char* const character_meshes = {"#- #-- # -# --#"};
static const char* const VaraqKeywords = {
    "woD latlh tam chImmoH qaw qawHa  ’ Hotlh pong cher HIja  ’ chugh ghobe  ’ "
    "chugh wIv"};
#define MAX_STR_LEN 100

// clang-format on
class lexer : public Lexilla::DefaultLexer {
    enum state : int {
        whitespace = 1,
        character = 228,
        identifier = 2,
    };
    Lexilla::WordList is_character_;

  public:
    lexer() : DefaultLexer("robotfindskitten", 228) {
        is_character_.Set(character_meshes);
    }
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length,
                        int initStyle, Scintilla::IDocument* pAccess) override {
        try {
            Lexilla::LexAccessor styler(pAccess);
            Lexilla::StyleContext ctx(startPos, length, initStyle, styler);
            Lexilla::CharacterSet setKeywordChars(
                Lexilla::CharacterSet::setAlpha, "#-");

            for (; ctx.More(); ctx.Forward()) {
                switch (ctx.state) {
                case state::identifier:
                    if (!setKeywordChars.Contains(ctx.ch)) {
                        auto tmpStr = new char[MAX_STR_LEN];
                        memset(tmpStr, 0, sizeof(char) * MAX_STR_LEN);
                        ctx.GetCurrent(tmpStr, MAX_STR_LEN);
                        if (is_character_.InList(tmpStr)) {
                            ctx.ChangeState(state::character);
                        };
                        ctx.SetState(state::whitespace);
                        delete[] tmpStr;
                    };
                    break;
                    break;
                case state::whitespace:
                default:
                    if (setKeywordChars.Contains(ctx.ch)) {
                        ctx.SetState(state::identifier);
                        break;
                    };
                    break;
                };
            }
            ctx.Complete();
        } catch (...) {
            std::cout << "error";
            // Should not throw into caller as may be compiled with different
            // compiler or options
            pAccess->SetErrorStatus(SC_STATUS_FAILURE);
        }
    }
    void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length,
                         int initStyle,
                         Scintilla::IDocument* pAccess) override {}
    virtual int SCI_METHOD GetIdentifier() override { return 21; }
    virtual const char* SCI_METHOD PropertyGet(const char* key) override {
        // props.Get(key);
        return "";
    };
};
