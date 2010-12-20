{-
 ----------------------------------------------------------------------------------
 -  Copyright (C) 2010  Massachusetts Institute of Technology
 -  Copyright (C) 2010  Yuan Tang <yuantang@csail.mit.edu>
 - 		                Charles E. Leiserson <cel@mit.edu>
 - 	 
 -   This program is free software: you can redistribute it and/or modify
 -   it under the terms of the GNU General Public License as published by
 -   the Free Software Foundation, either version 3 of the License, or
 -   (at your option) any later version.
 -
 -   This program is distributed in the hope that it will be useful,
 -   but WITHOUT ANY WARRANTY; without even the implied warranty of
 -   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 -   GNU General Public License for more details.
 -
 -   You should have received a copy of the GNU General Public License
 -   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 -
 -   Suggestsions:                  yuantang@csail.mit.edu
 -   Bugs:                          yuantang@csail.mit.edu
 -
 --------------------------------------------------------------------------------
 -}

module PBasicParser where

import Text.ParserCombinators.Parsec
import qualified Text.ParserCombinators.Parsec.Token as Token
import Text.ParserCombinators.Parsec.Expr
import Text.ParserCombinators.Parsec.Language
import Text.Read (read)

import Data.Char
import Data.List
import qualified Data.Map as Map

import PShow
import PUtils
import PData

{- all the token parsers -}
lexer :: Token.TokenParser st 
lexer = Token.makeTokenParser (javaStyle
             { commentStart = "/*",
               commentEnd = "*/",
               commentLine = "//",
               identStart = letter <|> oneOf "_'",
               identLetter = alphaNum <|> oneOf "_'", 
               nestedComments = True,
               reservedOpNames = ["*", "/", "+", "-", "!", "&&", "||", "=", ">", ">=", 
                                  "<", "<=", "==", "!=", "+=", "-=", "*=", "&=", "|=", 
                                  "<<=", ">>=", "^=", "++", "--", "?", ":", "&", "|", "~",
                                  ">>", "<<", "%", "^"],
               reservedNames = ["Pochoir_Array", "Pochoir", "Pochoir_Domain", 
                                "Pochoir", "Pochoir_pRange", 
                                "Pochoir_kernel_1D", "Pochoir_kernel_2D", 
                                "Pochoir_kernel_3D", "Pochoir_kernel_end",
                                "auto", "};", "const", "volatile", "register", 
                                "Pochoir_Boundary_1D", "Pochoir_Boundary_2D",
                                "Pochoir_Boundary_3D", "Pochoir_Boundary_end",
                                "#define", "int", "float", "double", "bool", "true", "false",
                                "if", "else", "switch", "case", "break", "default",
                                "while", "do", "for"],
               caseSensitive = True})

{- definition of all token parser -}
whiteSpace = Token.whiteSpace lexer
lexeme = Token.lexeme lexer
symbol = Token.symbol lexer
natural = Token.natural lexer
number = Token.naturalOrFloat lexer
integer = Token.integer lexer
brackets = Token.brackets lexer
braces = Token.braces lexer
parens = Token.parens lexer
angles = Token.angles lexer
semi = Token.semi lexer
colon = Token.colon lexer
dot = Token.dot lexer
identifier = Token.identifier lexer
reserved = Token.reserved lexer
reservedOp = Token.reservedOp lexer
comma = Token.comma lexer
semiSep = Token.semiSep lexer
semiSep1 = Token.semiSep1 lexer
commaSep = Token.commaSep lexer
commaSep1 = Token.commaSep1 lexer
charLiteral = Token.charLiteral lexer
stringLiteral = Token.stringLiteral lexer

-- pIdentifier doesn't strip whites, compared with 'identifier', 
-- so we can preserve the relative order of original source input
pIdentifier :: GenParser Char ParserState String
pIdentifier = do l_start <- letter <|> char '_'
                 l_body <- many (alphaNum <|> char '_' <?> "Wrong Identifier")
                 return (l_start : l_body)

pDelim :: GenParser Char ParserState String
pDelim = do try comma
            return ", "
     <|> do try semi
            return ";\n"
     <|> do try $ symbol ")"
            return ") "

pMember :: String -> GenParser Char ParserState String
pMember l_memFunc = 
    do l_start <- char '.'
       l_body <- string l_memFunc
       return (l_memFunc)

ppStencil :: String -> PStencil -> ParserState -> GenParser Char ParserState String
ppStencil l_id l_stencil l_state = 
        do try $ pMember "registerArray"
           l_array <- parens identifier
           semi
           case Map.lookup l_array $ pArray l_state of
               Nothing -> registerUndefinedArray l_id l_array l_stencil 
               Just l_pArray -> registerArray l_id l_array l_pArray
    <|> do try $ pMember "registerBoundaryFn"
           l_boundaryParams <- parens $ commaSep1 identifier
           semi
           let l_array = head l_boundaryParams
           let l_bdry = head $ tail l_boundaryParams 
           case Map.lookup l_array $ pArray l_state of
               Nothing -> registerUndefinedBoundaryFn l_id l_boundaryParams l_stencil
               Just l_pArray -> registerBoundaryFn l_id l_boundaryParams l_pArray
    <|> do try $ pMember "run"
           (l_tstep, l_func) <- parens pStencilRun
           semi
           case Map.lookup l_func $ pKernel l_state of
               Nothing -> return ("{" ++ breakline ++ l_id ++ ".run(" ++ 
                                   l_tstep ++ ", " ++ l_func ++ ");" ++ breakline ++ 
                                   "}" ++ breakline)
               Just l_kernel -> 
                    let l_revKernel = transKernel l_kernel l_stencil $ pMode l_state
                    in case pMode l_state of
                            PDefault -> 
                                let l_showKernel = 
                                       if sRank l_stencil < 3
                                           then pShowOptPointerKernel
                                           else pShowPointerKernel
                                in  pSplitObase
                                        ("Default_", l_id, l_tstep, l_revKernel, l_stencil)
                                        l_showKernel
                            PTypeShadow -> 
                                pSplitScope 
                                    ("type_", l_id, l_tstep, l_revKernel, l_stencil) 
                                    (pShowTypeKernel $ sArrayInUse l_stencil)
                            PMacroShadow -> 
                                pSplitScope 
                                    ("macro_", l_id, l_tstep, l_revKernel, l_stencil) 
                                    (pShowMacroKernel $ sArrayInUse l_stencil)
                            PInterior -> 
                                pSplitScope 
                                    ("interior_", l_id, l_tstep, l_revKernel, l_stencil) 
                                    pShowInteriorKernel
                            PIter -> 
                                pSplitObase 
                                    ("Iter_", l_id, l_tstep, l_revKernel, l_stencil) 
                                    pShowObaseKernel
                            PPointer -> 
                                pSplitObase 
                                    ("Pointer_", l_id, l_tstep, l_revKernel, l_stencil) 
                                    pShowPointerKernel
                            POptPointer -> 
                                pSplitObase 
                                    ("Opt_Pointer_", l_id, l_tstep, l_revKernel, l_stencil) 
                                    pShowOptPointerKernel
    <|> do return (l_id)

-- get all iterators from Kernel
transKernel :: PKernel -> PStencil -> PMode -> PKernel
transKernel l_kernel l_stencil l_mode =
       let l_exprStmts = kStmt l_kernel
           l_kernelParams = kParams l_kernel
           l_iters =
                   case l_mode of 
                       PTypeShadow -> getFromStmts getIter 
                                    (transArrayMap $ sArrayInUse l_stencil) 
                                    l_exprStmts
                       PMacroShadow -> getFromStmts getIter 
                                    (transArrayMap $ sArrayInUse l_stencil) 
                                    l_exprStmts
                       PInterior -> getFromStmts getIter 
                                    (transArrayMap $ sArrayInUse l_stencil) 
                                    l_exprStmts
                       PIter -> getFromStmts getIter 
                                    (transArrayMap $ sArrayInUse l_stencil) 
                                    l_exprStmts
                       PPointer -> getFromStmts 
                                    (getPointer $ l_kernelParams) 
                                    (transArrayMap $ sArrayInUse l_stencil) 
                                    l_exprStmts
                       POptPointer -> getFromStmts 
                                    getIter 
                                    (transArrayMap $ sArrayInUse l_stencil) 
                                    l_exprStmts 
                       PDefault -> let l_get = 
                                            if sRank l_stencil < 3 
                                                then getIter
                                                else (getPointer $ l_kernelParams)
                                   in  getFromStmts 
                                         l_get
                                         (transArrayMap $ sArrayInUse l_stencil) 
                                         l_exprStmts 
           l_revIters = transIterN 0 l_iters
       in  l_kernel { kIter = l_revIters }
 
pSplitScope :: (String, String, String, PKernel, PStencil) -> (String -> PKernel -> String) -> GenParser Char ParserState String
pSplitScope (l_tag, l_id, l_tstep, l_kernel, l_stencil) l_showKernel = 
    let oldKernelName = kName l_kernel
        obaseKernelName = l_tag ++ oldKernelName
        obaseKernel = l_showKernel obaseKernelName l_kernel
        runKernel = obaseKernelName ++ ", " ++ oldKernelName
    in  return ("{" ++ breakline ++ obaseKernel ++ breakline ++ l_id ++ ".run(" ++ 
               l_tstep ++ ", " ++ runKernel ++ ");" ++ breakline ++ "}" ++ 
               breakline)

pSplitObase :: (String, String, String, PKernel, PStencil) -> (String -> PKernel -> String) -> GenParser Char ParserState String
pSplitObase (l_tag, l_id, l_tstep, l_kernel, l_stencil) l_showKernel = 
    let oldKernelName = kName l_kernel 
        obaseKernelName = l_tag ++ oldKernelName 
        regBound = sRegBound l_stencil
        obaseKernel = l_showKernel obaseKernelName l_kernel 
        runKernel = 
            if regBound then obaseKernelName ++ ", " ++ oldKernelName
            -- if the boundary function is NOT registered, we guess user are using 
            -- zero-padding. Note: there's no zero-padding for Periodic stencils
                        else obaseKernelName
    in  return (obaseKernel ++ breakline ++ l_id ++ ".run_obase(" ++ l_tstep ++ 
                ", " ++ runKernel ++ ");" ++ breakline)

pStencilRun :: GenParser Char ParserState (String, String)
pStencilRun = 
        do l_tstep <- try exprStmtDim
           comma
           l_func <- identifier
           return (show l_tstep, l_func)
    <?> "Stencil Run Parameters"

registerUndefinedBoundaryFn :: String -> [String] -> PStencil -> GenParser Char ParserState String
registerUndefinedBoundaryFn l_id l_boundaryParams l_stencil =
    let l_arrayName = head l_boundaryParams
        l_pArray = PArray {aName = l_arrayName,
                           aType = sType l_stencil,
                           aRank = sRank l_stencil,
                           aDims = [],
                           aMaxShift = 0,
                           aToggle = sToggle l_stencil}
    in do -- updateState $ updatePArray [(l_arrayName, l_pArray)]
          -- updateState $ updateStencilArray l_id l_pArray
          -- updateState $ updateStencilBoundary l_id True
          return (l_id ++ ".registerBoundaryFn(" ++ (intercalate ", " l_boundaryParams) ++ 
                  "); /* register Undefined Boundary Fn */" ++ breakline)

registerBoundaryFn :: String -> [String] -> PArray -> GenParser Char ParserState String 
registerBoundaryFn l_id l_boundaryParams l_pArray =
    do updateState $ updateStencilArray l_id l_pArray
       updateState $ updateStencilBoundary l_id True
       return (l_id ++ ".registerBoundaryFn(" ++ (intercalate ", " l_boundaryParams) ++ 
               "); /* register Boundary Fn */" ++ breakline)

registerUndefinedArray :: String -> String -> PStencil -> GenParser Char ParserState String
registerUndefinedArray l_id l_arrayName l_stencil =
    let l_pArray = PArray {aName = l_arrayName,
                           aType = sType l_stencil,
                           aRank = sRank l_stencil,
                           aDims = [],
                           aMaxShift = 0,
                           aToggle = sToggle l_stencil}
    in  do -- updateState $ updatePArray [(l_arrayName, l_pArray)]
           -- updateState $ updateStencilArray l_id l_pArray 
           return (l_id ++ ".registerArray (" ++ l_arrayName ++ 
                   "); /* register Undefined Array */" ++ breakline)
    
registerArray :: String -> String -> PArray -> GenParser Char ParserState String
registerArray l_id l_arrayName l_pArray =
    do updateState $ updateStencilArray l_id l_pArray
       return (l_id ++ ".registerArray (" ++ l_arrayName ++ 
               "); /* register Array */" ++ breakline)

-- pDeclStatic <type, rank, toggle>
pDeclStatic :: GenParser Char ParserState (PType, PValue, PValue)
pDeclStatic = do l_type <- pType 
                 comma
                 -- exprDeclDim is an int that has to be known at compile-time
                 l_rank <- exprDeclDim
                 l_toggle <- option 2 (comma >> exprDeclDim)
                 return (l_type, l_rank, l_toggle)

pDeclStaticNum :: GenParser Char ParserState (PValue)
pDeclStaticNum = do l_rank <- exprDeclDim
                    return (l_rank)

pDeclDynamic :: GenParser Char ParserState ([PName], PName, [DimExpr])
pDeclDynamic = do (l_qualifiers, l_name) <- try pVarDecl
--                  l_dims <- parens (commaSep1 exprDeclDim)
                  -- exprStmtDim is something might be known at run-time
                  l_dims <- option [] (parens $ commaSep1 exprStmtDim)
                  return (l_qualifiers, l_name, l_dims)

pVarDecl :: GenParser Char ParserState ([PName], PName)
pVarDecl = do l_qualifiers <- many cppQualifier
              l_name <- identifier
              return (l_qualifiers, l_name)

cppQualifier :: GenParser Char ParserState PName
cppQualifier = 
       do reservedOp "*"
          return "*"
   <|> do reservedOp "&"
          return "&"
   <|> do reservedOp "&&"
          return "&&"
   <|> do reserved "const"
          return "const"
   <|> do reserved "volatile"
          return "volatile"
   <|> do reserved "register"
          return "register"

ppShape :: GenParser Char ParserState [Int]
ppShape = do l_shape <- braces (commaSep1 $ integer >>= return . fromInteger)
             return (l_shape)

{- parse a single statement which is ended by ';' -}
pStubStatement :: GenParser Char ParserState Stmt
pStubStatement = do stmt <- manyTill anyChar $ try eol 
                    return (UNKNOWN $ stmt)

{- parse a single statement which is ended by ';' 
 - new version : return the expr (syntax tree) instead of string
 -}
pStatement :: GenParser Char ParserState Stmt
pStatement = try pParenStmt 
         <|> try pDeclLocalStmt
         <|> try pIfStmt
         <|> try pSwitchStmt
         <|> try pWhileStmt
         <|> try pDoStmt
         <|> try pForStmt
         <|> try pExprStmt
         <|> try pNOPStmt
          -- pStubStatement scan in everything else except the "Pochoir_kernel_end" or "};"
         <|> try pStubStatement
         <?> "Statement"

pNOPStmt :: GenParser Char ParserState Stmt
pNOPStmt =
    do semi
       return NOP

pExprStmt :: GenParser Char ParserState Stmt
pExprStmt =
    do {- C++ comments are filtered by exprStmt -}
       l_expr <- try exprStmt
       semi
       return (EXPR l_expr)

pForStmt :: GenParser Char ParserState Stmt
pForStmt = 
    do reserved "for"
       l_exprs <- parens $ semiSep1 (commaSep pForExpr)
       l_stmt <- pStatement
       return (FOR l_exprs l_stmt)

pDoStmt :: GenParser Char ParserState Stmt
pDoStmt =
    do reserved "do"
       l_stmts <- braces (many pStatement)
       reserved "while"
       l_expr <- exprStmt
       semi
       return (DO l_expr l_stmts)

pWhileStmt :: GenParser Char ParserState Stmt
pWhileStmt =
    do reserved "while"
       l_boolExpr <- exprStmt
       l_stmts <- pStatement
       return (WHILE l_boolExpr l_stmts)

pParenStmt :: GenParser Char ParserState Stmt
pParenStmt =
    do symbol "{"
       l_stmts <- manyTill pStatement (try $ symbol "}")
       return (BRACES l_stmts)

pTypeDecl :: GenParser Char ParserState ([PName], PType)
pTypeDecl = do l_qualifiers <- many cppQualifier
               l_type <- pType
               return (l_qualifiers ++ [typeName l_type], l_type)

pTypeDecl_r :: GenParser Char ParserState ([PName], PType)
pTypeDecl_r = do l_type <- pType
                 l_qualifiers <- many cppQualifier
                 return ([typeName l_type] ++ l_qualifiers, l_type)

pDeclLocalStmt :: GenParser Char ParserState Stmt
pDeclLocalStmt =
    do (l_qualifiers, l_type) <- try pTypeDecl <|> try pTypeDecl_r
       l_exprs <- commaSep1 exprStmt 
       semi
       return (DEXPR l_qualifiers l_type l_exprs)

pIfStmt :: GenParser Char ParserState Stmt
pIfStmt =
    do reserved "if"
       l_boolExpr <- exprStmt
       l_trueBranch <- pStatement
       l_falseBranch <- option NOP pElseBranch
       return (IF l_boolExpr l_trueBranch l_falseBranch)

pSwitchStmt :: GenParser Char ParserState Stmt
pSwitchStmt =
    do reserved "switch"
       l_boolExpr <- exprStmt
       l_cases <- braces (many pCase)
       return (SWITCH l_boolExpr l_cases)
 
pParams :: GenParser Char ParserState (RegionT, Bool)
pParams = do l_regionT <- pRegionParam
             option "" comma
             l_obase <- option False (pObaseParam)
             return (l_regionT, l_obase)

pRegionParam :: GenParser Char ParserState RegionT
pRegionParam = do reserved "Periodic"
                  return Periodic
           <|> do reserved "Non-periodic"
                  return Nonperiodic
           <?> "Periodic/Non-periodic"

pObaseParam :: GenParser Char ParserState Bool
pObaseParam = do reserved "Obase"
                 return True
            <|>  return False
                  
pForExpr :: GenParser Char ParserState Stmt
pForExpr =  do (l_qualifiers, l_type) <- try pTypeDecl <|> try pTypeDecl_r
               l_exprs <- commaSep1 exprStmt 
               return (DEXPR l_qualifiers l_type l_exprs)
        <|> do l_expr <- exprStmt
               return (EXPR l_expr)
        <|> do whiteSpace
               return NOP
        <?> "For Expression"

pCase :: GenParser Char ParserState Stmt
pCase = do reserved "case"
           l_value <- natural >>= return . fromInteger
           colon
           l_stmts <- manyTill pStatement $ reserved "break"
           semi
           return (CASE l_value (l_stmts ++ [BREAK]))
    <|> do reserved "default"
           colon
           l_stmts <- manyTill pStatement $ reserved "break"
           semi
           return (DEFAULT (l_stmts ++ [BREAK]))
    <?> "Cases"

pElseBranch :: GenParser Char ParserState Stmt
pElseBranch = do reserved "else"
                 l_stmt <- pStatement
                 return l_stmt

{-
pSimpleType :: GenParser Char ParserState PType
pSimpleType = 
        do reserved "double" 
           return (PDouble)
    <|> do reserved "int"
           return (PInt)
    <|> do reserved "float"
           return (PFloat)
    <|> do reserved "bool"
           return (PBool)
-}

pType :: GenParser Char ParserState PType
pType = do reserved "double" 
           return PType{typeName = "double", basicType = PDouble}
    <|> do reserved "int"
           return PType{typeName = "int", basicType = PInt}
    <|> do reserved "float"
           return PType{typeName = "float", basicType = PFloat}
    <|> do reserved "bool"
           return PType{typeName = "bool", basicType = PBool}
    <|> do reserved "char"
           return PType{typeName = "char", basicType = PChar}
    <|> do reserved "short"
           return PType{typeName = "short", basicType = PShort}
    <|> do reserved "long"
           return PType{typeName = "long", basicType = PLong}
    <|> do reserved "unsigned"
           return PType{typeName = "unsigned", basicType = PUnsigned}
    <|> do reserved "signed"
           return PType{typeName = "signed", basicType = PSigned}
    <|> do reserved "void"
           return PType{typeName = "void", basicType = PVoid}
    <|> do l_type <- identifier
           l_qualifiers <- many cppQualifier
           return PType{typeName = l_type ++ (intercalate " " l_qualifiers), basicType = PUserType}
    <|> do l_qualifiers <- many cppQualifier
           l_type <- identifier 
           return PType{typeName = (intercalate " " l_qualifiers) ++ l_type, basicType = PUserType}

eol :: GenParser Char ParserState String
eol = do string "\n" 
         whiteSpace
         return "\n"
  <|> do string "\r\n" 
         whiteSpace
         return "\r\n"
  <|> do string "\n\r" 
         whiteSpace
         return "\n\r"
  <?> "eol"

-- Expression Parser for Dim in Declaration --
exprDeclDim :: GenParser Char ParserState Int
exprDeclDim = buildExpressionParser tableDeclDim termDeclDim
   <?> "exprDeclDim"

tableDeclDim = [[Prefix (reservedOp "-" >> return negate)],
         [op "*" (*) AssocLeft, op "/" div AssocLeft],
         [op "+" (+) AssocLeft, op "-" (-) AssocLeft]]
         where op s fop assoc = Infix (do {reservedOp s; return fop} <?> "operator") assoc

termDeclDim :: GenParser Char ParserState Int
termDeclDim = 
       try (parens exprDeclDim)
{-
   <|> do literal_dim <- try (identifier)
          l_state <- getState
          case Map.lookup literal_dim $ pMacro l_state of
              -- FIXME: If it's nothing, then something must be wrong
              Nothing -> return (0)
              Just num_dim -> return (num_dim)
-}
   <|> do num_dim <- try (natural)
          return (fromInteger num_dim)
   <?> "termDeclDim"

-- Expression Parser for Dim in Statements --
exprStmtDim :: GenParser Char ParserState DimExpr
exprStmtDim = buildExpressionParser tableStmtDim termStmtDim <?> "ExprStmtDim"

tableStmtDim = [
         [bop "*" "*" AssocLeft, bop "/" "/" AssocLeft],
         [bop "+" "+" AssocLeft, bop "-" "-" AssocLeft],
         [bop "==" "==" AssocLeft, bop "!=" "!=" AssocLeft]]
         where bop str fop assoc = Infix ((reservedOp str >> return (DimDuo fop)) <?> "operator") assoc

termStmtDim :: GenParser Char ParserState DimExpr
termStmtDim = do e <- try (parens exprStmtDim)
                 return (DimParen e)
          <|> do literal_dim <- try (identifier)
                 return (DimVAR literal_dim)
{-
                 l_state <- getState
                 -- check whether it's an effective Range name
                 case Map.lookup literal_dim $ pMacro l_state of
                     Nothing -> return (DimVAR literal_dim)
                     Just l_dim -> return (DimINT l_dim)
-}
          <|> do num_dim <- try (natural)
                 return (DimINT $ fromInteger num_dim)
          <?> "TermStmtDim"

-- Expression Parser for Statements --
exprStmt :: GenParser Char ParserState Expr
exprStmt = buildExpressionParser tableStmt termStmt
   <?> "Expression Statement"

tableStmt = [[Postfix (reservedOp "++" >> return (PostUno "++")),
              Postfix (reservedOp "--" >> return (PostUno "--"))],
            [Prefix (reservedOp "!" >> return (Uno "!")),
              Prefix (reservedOp "~" >> return (Uno "~")),
              Prefix (reservedOp "++" >> return (Uno "++")),
              Prefix (reservedOp "--" >> return (Uno "--")),
              Prefix (reservedOp "-" >> return (Uno "-")),
              Prefix (reservedOp "+" >> return (Uno "+")), --Unary Plus
              Prefix (reservedOp "*" >> return (Uno "*")), --Dereference
              Prefix (reservedOp "&" >> return (Uno "&"))  --Address of
             ],
         [op "*" "*" AssocLeft, op "/" "/" AssocLeft,
          op "%" "%" AssocLeft],
         [op "+" "+" AssocLeft, op "-" "-" AssocLeft],
         [op ">>" ">>" AssocLeft, op "<<" "<<" AssocLeft],
         [op ">" ">" AssocLeft, op "<" "<" AssocLeft,
          op ">=" ">=" AssocLeft, op "<=" "<=" AssocLeft],
         [op "==" "==" AssocLeft, op "!=" "!=" AssocLeft],
         [op "&" "&" AssocLeft], --bitwise and
         [op "^" "^" AssocLeft], --bitwise xor
         [op "|" "|" AssocLeft], --bitwise inclusive or
         [op "&&" "&&" AssocLeft], --logical and
         [op "||" "||" AssocLeft], --logical or
         [op ":" ":" AssocLeft],
         [op "?" "?" AssocLeft],
         [Infix (reservedOp "=" >> return (Duo "=")) AssocLeft,
          Infix (reservedOp "/=" >> return (Duo "/=")) AssocLeft,
          Infix (reservedOp "*=" >> return (Duo "*=")) AssocLeft,
          Infix (reservedOp "+=" >> return (Duo "+=")) AssocLeft,
          Infix (reservedOp "-=" >> return (Duo "-=")) AssocLeft,
          Infix (reservedOp "%=" >> return (Duo "%=")) AssocLeft,
          Infix (reservedOp "&=" >> return (Duo "&=")) AssocLeft,
          Infix (reservedOp "|=" >> return (Duo "|=")) AssocLeft,
          Infix (reservedOp "^=" >> return (Duo "^=")) AssocLeft,
          Infix (reservedOp ">>=" >> return (Duo ">>=")) AssocLeft,
          Infix (reservedOp "<<=" >> return (Duo "<<=")) AssocLeft
          ]]
         where op s fop assoc = Infix (do {reservedOp s; return (Duo fop)} <?> "operator") assoc

termStmt :: GenParser Char ParserState Expr
termStmt =  do try pArrayOfStructTermStmt
        <|> do try ppArrayOfStructTermStmt
--        <|> do try pPArrayOfStructTermStmt
        <|> do l_expr <- try (parens exprStmt) 
               return (PARENS l_expr)
        <|> do l_num <- try (number)
               case l_num of
                  Left n -> return (INT $ fromInteger n)
                  Right n -> return (FLOAT n)
        <|> do reserved "true" 
               return (BOOL "true") 
        <|> do reserved "false"
               return (BOOL "false")
        <|> do try pParenTermStmt
        <|> do try pBracketTermStmt
        <|> do try pBExprTermStmt
        -- pArrayOfStructTermStmt has to be before pPlainVarTermStmt
        -- because pPlainVarTermStmt just scan a plain identifier
        -- so, it's a conflict with pArrayOfStructTermStmt
        <|> do try pPlainVarTermStmt
        <?> "term statement"

pParenTermStmt :: GenParser Char ParserState Expr
pParenTermStmt =
    do l_qualifiers <- try (many cppQualifier)
       l_var <- try identifier
       l_dims <- parens (commaSep1 exprStmtDim)
       return (PVAR (concat l_qualifiers) l_var l_dims)

pBracketTermStmt :: GenParser Char ParserState Expr
pBracketTermStmt =
    do l_var <- try identifier
       l_dim <- brackets exprStmtDim
       return (BVAR l_var l_dim)

pBExprTermStmt :: GenParser Char ParserState Expr
pBExprTermStmt =
    do l_var <- try identifier
       l_expr <- brackets exprStmt
       return (BExprVAR l_var l_expr)

pPlainVarTermStmt :: GenParser Char ParserState Expr
pPlainVarTermStmt =
    do l_qualifiers <- try (many cppQualifier)
       l_var <- try identifier
       return (VAR (concat l_qualifiers) l_var)

pArrayOfStructTermStmt :: GenParser Char ParserState Expr
pArrayOfStructTermStmt =
    do l_type <- try pType
       l_pTerm <- parens pParenTermStmt
       l_connector <- symbol "." <|> symbol "->"
       l_field <- identifier
       return (SVAR l_type l_pTerm l_connector l_field)

ppArrayOfStructTermStmt :: GenParser Char ParserState Expr
ppArrayOfStructTermStmt =
    do let opt_type = PType{basicType = PUserType, typeName = ""}
       l_pTerm <- pParenTermStmt 
       l_connector <- symbol "." <|> symbol "->"
       l_field <- identifier
       return (SVAR opt_type l_pTerm l_connector l_field)

pPArrayOfStructTermStmt :: GenParser Char ParserState Expr
pPArrayOfStructTermStmt =
    do l_type <- parens pType
       l_pTerm <- parens pParenTermStmt
       l_connector <- symbol "." <|> symbol "->"
       l_field <- identifier
       return (PSVAR l_type l_pTerm l_connector l_field)
