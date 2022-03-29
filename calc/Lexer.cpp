#include "Lexer.h"

namespace charinfo {
LLVM_READNONE inline bool isWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\f' || c == '\v' ||
         c == '\r' || c == '\n';
}

LLVM_READNONE inline bool isDigit(char c) {
  return c >= '0' && c <= '9';
}
} // namespace charinfo

void Lexer::next(Token &token) {
  while (*BufferPtr && charinfo::isWhitespace(*BufferPtr)) {
    ++BufferPtr;
  }

  if (!*BufferPtr) {
    token.Kind = Token::eoi;
    return;
  }

  if (charinfo::isDigit(*BufferPtr)) {
    const char *end = BufferPtr + 1;
    while (charinfo::isDigit(*end))
      ++end;
    formToken(token, end, Token::number);
    return;
  } else {
    switch (*BufferPtr) {
#define CASE(ch, tok)                     \
  case ch:                                \
    formToken(token, BufferPtr + 1, tok); \
    break;
      CASE('+', Token::plus);
      CASE('-', Token::minus);
#undef CASE
    default:
      formToken(token, BufferPtr + 1, Token::unknown);
    }
    return;
  }
}

void Lexer::formToken(Token &Tok, const char *TokEnd,
                      Token::TokenKind Kind) {
  Tok.Kind = Kind;
  Tok.Text = llvm::StringRef(BufferPtr, TokEnd - BufferPtr);
  BufferPtr = TokEnd;
}
