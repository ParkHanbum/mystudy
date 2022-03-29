#include "Parser.h"

AST *Parser::parse() {
	AST *Res = parseFomula();
	expect(Token::eoi);
	return Res;
}

AST *Parser::parseFomula() {
	Expr *E;

	E = parseExpr();
	if (expect(Token::eoi))
		goto _error;
	
	return E;

_error:
	while (Tok.getKind() != Token::eoi)
		advance();

	return nullptr;
}

Expr *Parser::parseExpr() {
	Expr *Left = parseFactor();
	while (Tok.isOneOf(Token::plus, Token::minus)) {
		BinaryOp::Operator Op = Tok.is(Token::plus)
																? BinaryOp::Plus
																: BinaryOp::Minus;
		advance();
		Expr *Right = parseFactor();
		Left = new BinaryOp(Op, Left, Right);
	}

	return Left;
}

Expr *Parser::parseFactor() {
	Expr *Res = nullptr;
	Res = new Factor(Factor::Number, Tok.getText());
	advance();
	return Res;
}
