#pragma once
#ifndef _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_LEXER_H
#define _CPP_PROJECTS_ROBOTFINDSKITTEN_SRC_ROBOTFINDSKITTEN_GAME_LEXER_H

#include <array>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>

#include <Windows.h>

// should inludes alls in this order
// clang-format off
#include <ILexer.h>
#include <Scintilla.h>
#include <SciLexer.h>
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


#include "config.h"
// clang-format on

static inline constexpr int MY_STYLE_START = 33;
[[maybe_unused]] static inline constexpr std::pair<int, int> PRINTABLE_RANGE{
    static_cast<int>(' '), static_cast<int>('~')};

class lexer: public Lexilla::DefaultLexer {
public:
    lexer(): DefaultLexer(PROJECT_NAME, static_cast<int>('#')) {}
    void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length,
                        int initStyle, Scintilla::IDocument* pAccess) override {
        try {
            Lexilla::LexAccessor styler(pAccess);
            Lexilla::StyleContext ctx(startPos, length, initStyle, styler);

            for(; ctx.More(); ctx.Forward()) {
                if(ctx.Match('#', '-') || ctx.Match('-', '#')) {
                    ctx.SetState(STYLE_DEFAULT);
                    ctx.Forward();
                    ctx.SetState(STYLE_DEFAULT);
                } else if(ctx.ch == '#') {
                    ctx.SetState(STYLE_DEFAULT);
                } else {
                    ctx.SetState((ctx.ch) + MY_STYLE_START);
                }
            }
            ctx.Complete();
        } catch(...) {
            // Should not throw into caller as may be compiled with different
            // compiler or options
            pAccess->SetErrorStatus(SC_STATUS_FAILURE);
        }
    }
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length,
                         int initStyle,
                         Scintilla::IDocument* pAccess) override {}
    int SCI_METHOD GetIdentifier() override {
        return static_cast<int>('#');
    }
    const char* SCI_METHOD PropertyGet(const char* /*key*/) override {
        return "";
    };
};

#endif
