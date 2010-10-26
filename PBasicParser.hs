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

import PData

{- all the token parsers -}
lexer :: Token.TokenParser st 
lexer = Token.makeTokenParser (javaStyle
             { commentStart = "/*",
               commentEnd = "*/",
               commentLine = "//",
               identStart = letter,
               identLetter = alphaNum <|> oneOf "_'", 
               nestedComments = True,
               reservedOpNames = ["*", "/", "+", "-", "!", "&&", "||", "=", ">", ">=", 
                                  "<", "<=", "==", "+=", "-=", "*=", "/=", "&=", "|=",
                                  "<<=", ">>=", "^=", "++", "--"],
               reservedNames = ["Pochoir_Array", "Pochoir_Stencil", "Pochoir_uRange", 
                                "Pochoir_Shape_info", "Pochoir_pRange", 
                                "Pochoir_kernel_1D", "Pochoir_kernel_2D", 
                                "Pochoir_kernel_3D", "Pochoir_kernel_end",
                                "auto", "};", 
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

pMember :: String -> GenParser Char ParserState String
pMember l_memFunc = 
    do l_start <- char '.'
       l_body <- string l_memFunc
       return (l_memFunc)

ppStencil :: String -> PStencil -> ParserState -> GenParser Char ParserState String
ppStencil l_id l_stencil l_state = 
        do try $ pMember "registerArrayInUse"
           l_array <- parens identifier
           semi
           case Map.lookup l_array $ pArray l_state of
               Nothing -> return (l_id ++ ".registerArrayInUse(" ++ l_array ++ "); /* ERROR!!! */" ++ breakline)
               Just l_pArray -> registerArrayInUse l_id l_array l_pArray
    <|> do try $ pMember "registerBoundaryFn"
           l_boundaryParams <- parens $ commaSep1 identifier
           semi
           updateState $ updateStencilBoundary l_id True
           return (l_id ++ ".registerBoundaryFn(" ++ (intercalate ", " l_boundaryParams) ++ ");" ++ breakline)
    <|> do try $ pMember "run"
           (l_tstep, l_func) <- parens pStencilRun
           semi
           case Map.lookup l_func $ pKernel l_state of
               Nothing -> return ("{" ++ breakline ++ l_id ++ ".run(" ++ 
                                   l_tstep ++ ", " ++ l_func ++ ");" ++ breakline ++ 
                                   "}" ++ breakline)
               Just l_kernel -> 
                    case pMode l_state of
                        PTypeShadow -> pSplitTypeShadow (l_id, l_tstep, l_kernel, l_stencil)
                        PMacroShadow -> pSplitMacroShadow (l_id, l_tstep, l_kernel, l_stencil)
                        PIter -> pSplitIter  (l_id, l_tstep, l_kernel, l_stencil)
                        PInterior -> pSplitInterior (l_id, l_tstep, l_kernel, l_stencil)
                        PPointer -> pSplitPointer (l_id, l_tstep, l_kernel, l_stencil)
    <|> do return (l_id)

pSplitTypeShadow :: (String, String, PKernel, PStencil) -> GenParser Char ParserState String
pSplitTypeShadow (l_id, l_tstep, l_kernel, l_stencil) = 
    let shadowKernel = pShowAutoKernel shadowKernelName l_kernel 
        oldKernelName = kName l_kernel
        shadowKernelName = "interior_type_shadow_" ++ oldKernelName
        shadowArrayInUse = pShowShadowArrayInUse $ sArrayInUse l_stencil
    in  return ("{" ++ shadowArrayInUse ++
               shadowKernel ++ breakline ++ l_id ++ ".run(" ++ 
               l_tstep ++ ", " ++ shadowKernelName ++ ", " ++ 
               oldKernelName ++ ");" ++ breakline ++ "}" ++ breakline)

pSplitMacroShadow :: (String, String, PKernel, PStencil) -> GenParser Char ParserState String
pSplitMacroShadow (l_id, l_tstep, l_kernel, l_stencil) = 
    let shadowKernel = pShowAutoKernel shadowKernelName l_kernel 
        oldKernelName = kName l_kernel
        shadowKernelName = "interior_macro_shadow_" ++ oldKernelName
        shadowArrayInUse = pDefMacroArrayInUse (sArrayInUse l_stencil) (kParams l_kernel)
        unshadowArrayInUse = pUndefMacroArrayInUse (sArrayInUse l_stencil) (kParams l_kernel)
    in  return ("{" ++ breakline ++ shadowArrayInUse ++
               shadowKernel ++ breakline ++ unshadowArrayInUse ++ 
               l_id ++ ".run(" ++ l_tstep ++ ", " ++ shadowKernelName ++ ", " ++ 
               oldKernelName ++ ");" ++ breakline ++ "}" ++ breakline)

pSplitInterior :: (String, String, PKernel, PStencil) -> GenParser Char ParserState String
pSplitInterior (l_id, l_tstep, l_kernel, l_stencil) =
    let oldKernelName = kName l_kernel
        interiorKernelName = "interior_" ++ oldKernelName
        interiorStmts = transStmts (kStmt l_kernel)
                            $ transInterior $ getArrayName $ sArrayInUse l_stencil
        interiorKernel = PKernel { kName = interiorKernelName,
                                   kParams = kParams l_kernel,
                                   kStmt = interiorStmts,
                                   kIter = kIter l_kernel 
                                  }
        showInteriorKernel = pShowAutoKernel interiorKernelName interiorKernel
    in  return (showInteriorKernel ++ l_id ++ ".run(" ++
                l_tstep ++ ", " ++ interiorKernelName ++ ", " ++ 
                oldKernelName ++ ");" ++ breakline ++ breakline)
        
transInterior :: [PName] -> Expr -> Expr
transInterior l_arrayInUse (PVAR v dL) =
    if elem v l_arrayInUse == True then PVAR (v ++ ".interior") dL
                                   else PVAR v dL
transInterior l_arrayInUse e = e

getArrayName :: [PArray] -> [PName]
getArrayName [] = []
getArrayName (a:as) = (aName a) : (getArrayName as)

pSplitIter :: (String, String, PKernel, PStencil) -> GenParser Char ParserState String
pSplitIter (l_id, l_tstep, l_kernel, l_stencil) = 
    do let oldKernelName = kName l_kernel 
       let obaseKernelName = "obase_" ++ oldKernelName 
       let regBound = sRegBound l_stencil
       let obaseKernel = pShowObaseKernel obaseKernelName l_kernel 
       let runKernel = 
            if regBound then obaseKernelName ++ ", " ++ oldKernelName
            -- if the boundary function is NOT registered, we guess user are using 
            -- zero-padding. Note: there's no zero-padding for Periodic stencils
                        else obaseKernelName
       return (obaseKernel ++ breakline ++ l_id ++ ".run_obase(" ++ l_tstep ++ ", " ++ runKernel ++ ");" ++ breakline)

pSplitPointer :: (String, String, PKernel, PStencil) -> GenParser Char ParserState String
pSplitPointer (l_id, l_tstep, l_kernel, l_stencil) =
    let oldKernelName = kName l_kernel
        newKernelName = "pointer_" ++ oldKernelName
        regBound = sRegBound l_stencil
        newKernel = pShowPointerKernel newKernelName l_kernel
        runKernel = 
            if regBound then newKernelName ++ ", " ++ oldKernelName
            -- if the boundary function is NOT registered, we guess user are using 
            -- zero-padding. Note: there's no zero-padding for Periodic stencils
                        else newKernelName 
    in  return (newKernel ++ breakline ++ l_id ++ ".run_obase(" ++ l_tstep ++ ", " ++ runKernel ++ ");" ++ breakline)

pStencilRun :: GenParser Char ParserState (String, String)
pStencilRun = 
        do l_tstep <- try exprStmtDim
           comma
           l_func <- identifier
           return (show l_tstep, l_func)
    <?> "Stencil Run Parameters"

registerArrayInUse :: String -> String -> PArray -> GenParser Char ParserState String
registerArrayInUse l_id l_array l_pArray =
    do updateState $ updateStencilArray l_id l_pArray
       return (l_id ++ ".registerArrayInUse (" ++ l_array ++ ");" ++ breakline)

updateStencilArray :: String -> PArray -> ParserState -> ParserState
updateStencilArray l_id l_pArray parserState =
    let f k x = 
            if sName x == l_id 
                then Just $ x { sArrayInUse = union [l_pArray] (sArrayInUse x) }
                else Nothing
    in  parserState { pStencil = Map.updateWithKey f l_id $ pStencil parserState }

updateStencilBoundary :: String -> Bool -> ParserState -> ParserState
updateStencilBoundary l_id l_regBound parserState =
    let f k x =
            if sName x == l_id
                then Just $ x { sRegBound = l_regBound }
                else Nothing
    in  parserState { pStencil = Map.updateWithKey f l_id $ pStencil parserState }

-- pDeclStatic <type, rank, toggle>
pDeclStatic :: GenParser Char ParserState (PType, PValue, PValue)
pDeclStatic = do l_type <- pType 
                 comma
                 -- exprDeclDim is something has to be known at compile-time
                 l_rank <- exprDeclDim
                 l_toggle <- option 2 (comma >> exprDeclDim)
                 return (l_type, l_rank, l_toggle)

pDeclStaticNum :: GenParser Char ParserState (PValue)
pDeclStaticNum = do l_rank <- exprDeclDim
                    return (l_rank)

pDeclDynamic :: GenParser Char ParserState (PName, [DimExpr])
pDeclDynamic = do l_name <- identifier
--                  l_dims <- parens (commaSep1 exprDeclDim)
                  -- exprStmtDim is something might be known at run-time
                  l_dims <- parens (commaSep1 exprStmtDim)
                  return (l_name, l_dims)

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

pDeclLocalStmt :: GenParser Char ParserState Stmt
pDeclLocalStmt =
    do l_decl <- try pType
       l_exprs <- commaSep1 exprStmt 
       semi
       return (DEXPR l_decl l_exprs)

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
pForExpr =  do l_decl <- try pType
               l_exprs <- commaSep1 exprStmt 
               return (DEXPR l_decl l_exprs)
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

pType :: GenParser Char ParserState PType
pType = do reserved "double" 
           return PDouble
    <|> do reserved "int"
           return PInt
    <|> do reserved "float"
           return PFloat
    <|> do reserved "bool"
           return PBool
    <?> "Type"

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
termDeclDim = try (parens exprDeclDim)
   <|> do literal_dim <- try (identifier)
          l_state <- getState
          case Map.lookup literal_dim $ pMacro l_state of
              -- FIXME: If it's nothing, then something must be wrong
              Nothing -> return (0)
              Just num_dim -> return (num_dim)
   <|> do num_dim <- try (natural)
          return (fromInteger num_dim)
   <?> "termDeclDim"

-- Expression Parser for Dim in Statements --
exprStmtDim :: GenParser Char ParserState DimExpr
exprStmtDim = buildExpressionParser tableStmtDim termStmtDim <?> "ExprStmtDim"

tableStmtDim = [
         [bop "*" "*" AssocLeft, bop "/" "/" AssocLeft],
         [bop "+" "+" AssocLeft, bop "-" "-" AssocLeft]]
         where bop str fop assoc = Infix ((reservedOp str >> return (DimDuo fop)) <?> "operator") assoc

termStmtDim :: GenParser Char ParserState DimExpr
termStmtDim = do e <- try (parens exprStmtDim)
                 return (DimParen e)
          <|> do literal_dim <- try (identifier)
                 l_state <- getState
                 -- check whether it's an effective Range name
                 case Map.lookup literal_dim $ pMacro l_state of
                     Nothing -> return (DimVAR literal_dim)
                     Just l_dim -> return (DimINT l_dim)
          <|> do num_dim <- try (natural)
                 return (DimINT $ fromInteger num_dim)
          <?> "TermStmtDim"

-- Expression Parser for Statements --
exprStmt :: GenParser Char ParserState Expr
exprStmt = buildExpressionParser tableStmt termStmt
   <?> "Expression Statement"

tableStmt = [[Prefix (reservedOp "-" >> return (Uno "-")),
              Prefix (reservedOp "!" >> return (Uno "!")),
              Prefix (reservedOp "++" >> return (Uno "++")),
              Prefix (reservedOp "--" >> return (Uno "--")),
              Postfix (reservedOp "++" >> return (PostUno "++")),
              Postfix (reservedOp "--" >> return (PostUno "--"))
              ],
         [op "*" "*" AssocLeft, op "/" "/" AssocLeft],
         [op "+" "+" AssocLeft, op "-" "-" AssocLeft],
         [op ">" ">" AssocLeft, op "<" "<" AssocLeft,
          op ">=" ">=" AssocLeft, op "<=" "<=" AssocLeft,
          op "==" "==" AssocLeft],
         [op "&&" "&&" AssocLeft, op "||" "||" AssocLeft],
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
termStmt =  do l_expr <- try (parens exprStmt) 
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
        <|> do try pPlainVarTermStmt
        <?> "term statement"

pParenTermStmt :: GenParser Char ParserState Expr
pParenTermStmt =
    do l_var <- try identifier
       l_dims <- parens (commaSep1 exprStmtDim)
       return (PVAR l_var l_dims)

pBracketTermStmt :: GenParser Char ParserState Expr
pBracketTermStmt =
    do l_var <- try identifier
       l_dim <- brackets exprStmtDim
       return (BVAR l_var l_dim)

pPlainVarTermStmt :: GenParser Char ParserState Expr
pPlainVarTermStmt =
    do l_var <- try identifier
       return (VAR l_var)
