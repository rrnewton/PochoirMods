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

module PMainParser where

import Text.ParserCombinators.Parsec

import Control.Monad

import PBasicParser
import PParser2
import PUtils
import PData
import PShow
import qualified Data.Map as Map

pParser :: GenParser Char ParserState String
pParser = do tokens0 <- many $ pToken
             eof
             return $ concat tokens0
             -- start a second pass!
--             setInput $ concat tokens0
--             tokens1 <- many pToken1
--             return $ concat tokens1

pToken :: GenParser Char ParserState String
pToken = 
        try pParseCPPComment
    <|> try pParseMacro
    <|> try pParsePochoirArray
    <|> try pParsePochoirStencil
    <|> try pParsePochoirShapeInfo
    <|> try pParsePochoirDomain
    <|> try pParsePochoirKernel1D
    <|> try pParsePochoirKernel2D
    <|> try pParsePochoirKernel3D
    <|> try pParsePochoirAutoKernel
    <|> try pParsePochoirStencilMember
    <|> do ch <- anyChar
           return [ch]
    <?> "line"

pParseMacro :: GenParser Char ParserState String
pParseMacro = 
    do reserved "#define"
       l_name <- identifier
       pMacroValue l_name

pParsePochoirArray :: GenParser Char ParserState String
pParsePochoirArray =
    do reserved "Pochoir_Array"
       (l_type, l_rank, l_toggle) <- angles $ try pDeclStatic
       l_arrayDecl0 <- pDeclDynamic
--       let l_arrayDecl = [l_arrayDecl0]
--     'many' does NOT have the semantic of rolling back when the parser fails!!!
       l_arrayDecl1 <- many $ try $ comma >> pDeclDynamic 
       l_delim <- pDelim 
       let l_arrayDecl = [l_arrayDecl0] ++ l_arrayDecl1
--       l_arrayDecl <- commaSep1 pDeclDynamic
--       semi
       updateState $ updatePArray $ transPArray (l_type, l_rank, l_toggle) l_arrayDecl
       return (breakline ++ "/* Known*/ Pochoir_Array <" ++ show l_type ++ 
               ", " ++ show l_rank ++ ", " ++ show l_toggle ++ "> " ++ 
               pShowArrayDynamicDecl l_arrayDecl ++ l_delim)
       -- ++ ";\n")

pParsePochoirStencil :: GenParser Char ParserState String
pParsePochoirStencil = 
    do reserved "Pochoir"
       (l_type, l_rank, l_toggle) <- angles $ try pDeclStatic 
       l_stencil0 <- pDeclDynamic
       l_stencil1 <- many $ try $ comma >> pDeclDynamic
       l_delim <- pDelim
       let l_rawStencils = [l_stencil0] ++ l_stencil1
       let l_stencils = [pSecond l_stencil0] ++ map pSecond l_stencil1
--       l_stencils <- commaSep1 identifier 
--       semi
       updateState $ updatePStencil $ transPStencil (l_type, l_rank, l_toggle) l_stencils
       return (breakline ++ "/* Known */ Pochoir <" ++ show l_type ++ ", " ++ show l_rank ++ 
               ", " ++ show l_toggle ++ "> " ++ pShowArrayDynamicDecl l_rawStencils ++ 
               l_delim)
--               ", " ++ show l_toggle ++ "> " ++ pShowListIdentifiers l_stencils ++ 
--               ";\n")

pParsePochoirShapeInfo :: GenParser Char ParserState String
pParsePochoirShapeInfo = 
    do reserved "Pochoir_Shape"
       l_rank <- angles pDeclStaticNum
       l_name <- identifier
       l_len <- brackets pDeclStaticNum
       reservedOp "="
       l_shapes <- braces (commaSep1 ppShape)
       semi
       updateState $ updatePShape (l_name, l_rank, l_len, l_shapes)
       return (breakline ++ "Pochoir_Shape <" ++ show l_rank ++ "> " ++ l_name ++ " [" ++ show l_len ++ "] = " ++ pShowShapes l_shapes ++ ";\n")

pParsePochoirDomain :: GenParser Char ParserState String
pParsePochoirDomain =
    do reserved "Pochoir_Domain"
       l_rangeDecl0 <- pDeclDynamic
       l_rangeDecl1 <- many $ try $ comma >> pDeclDynamic
       let l_rangeDecl = [l_rangeDecl0] ++ l_rangeDecl1
       semi
       updateState $ updatePRange $ transURange l_rangeDecl
       return (breakline ++ "Pochoir_Domain " ++ pShowArrayDynamicDecl l_rangeDecl ++ ";\n")

pParsePochoirKernel1D :: GenParser Char ParserState String
pParsePochoirKernel1D =
    do reserved "Pochoir_kernel_1D"
       pPochoirKernel

pParsePochoirKernel2D :: GenParser Char ParserState String
pParsePochoirKernel2D =
    do reserved "Pochoir_kernel_2D"
       pPochoirKernel

pParsePochoirKernel3D :: GenParser Char ParserState String
pParsePochoirKernel3D =
    do reserved "Pochoir_kernel_3D"
       pPochoirKernel

pParsePochoirAutoKernel :: GenParser Char ParserState String
pParsePochoirAutoKernel =
    do reserved "auto"
       pPochoirAutoKernel

pParsePochoirStencilMember :: GenParser Char ParserState String
pParsePochoirStencilMember =
    do l_id <- try (pIdentifier)
       l_state <- getState
       case Map.lookup l_id $ pStencil l_state of
           Nothing -> return (l_id)
           Just l_stencil -> ppStencil l_id l_stencil l_state

pParseCPPComment :: GenParser Char ParserState String
pParseCPPComment =
        do try (string "/*")
           str <- manyTill anyChar (try $ string "*/")
           -- return ("/* comment */")
           return ("/*" ++ str ++ "*/")
    <|> do try (string "//")
           str <- manyTill anyChar (try $ eol)
           -- return ("// comment\n")
           return ("//" ++ str ++ "\n")

pPochoirKernel :: GenParser Char ParserState String
pPochoirKernel = 
    do  l_kernel_params <- parens $ commaSep1 identifier           
        exprStmts <- manyTill pStatement (try $ reserved "Pochoir_kernel_end")
        l_state <- getState
        let l_iters = 
                    if pMode l_state == PIter 
                        then getFromStmts getIter (pArray l_state) exprStmts
                        else getFromStmts (getPointer $ tail l_kernel_params) (pArray l_state) exprStmts
        let l_revIters = transIterN 0 l_iters
        let l_kernel = PKernel { kName = head l_kernel_params, 
                         kParams = tail l_kernel_params,
                         kStmt = exprStmts, kIter = l_revIters }
        updateState $ updatePKernel l_kernel
        return (pShowKernel (kName l_kernel) l_kernel)

pPochoirAutoKernel :: GenParser Char ParserState String
pPochoirAutoKernel =
    do l_kernel_name <- identifier
       reservedOp "="
       symbol "[&]"
       l_kernel_params <- parens $ commaSep1 (reserved "int" >> identifier)
       symbol "{"
       exprStmts <- manyTill pStatement (try $ reserved "};")
       let l_kernel = PKernel { kName = l_kernel_name, kParams = l_kernel_params,
                                kStmt = exprStmts, kIter = [] }
       updateState $ updatePKernel l_kernel
       return (pShowAutoKernel l_kernel_name l_kernel) 

pMacroValue :: String -> GenParser Char ParserState String
pMacroValue l_name = 
                -- l_value <- liftM fromInteger $ try (natural)
              do l_value <- try (natural) >>= return . fromInteger
           -- Because Macro is usually just 1 line, we omit the state update 
                 updateState $ updatePMacro (l_name, l_value)
                 return ("#define " ++ l_name ++ " " ++ show (l_value) ++ "\n")
          <|> do l_value <- manyTill anyChar $ try eol
                 return ("#define " ++ l_name ++ " " ++ l_value ++ "\n")
          <?> "Macro Definition"

transPArray :: (PType, Int, Int) -> [([PName], PName, [DimExpr])] -> [(PName, PArray)]
transPArray (l_type, l_rank, l_toggle) [] = []
transPArray (l_type, l_rank, l_toggle) (p:ps) =
    let l_name = pSecond p
        l_dims = pThird p
    in  (l_name, PArray {aName = l_name, aType = l_type, aRank = l_rank, aDims = l_dims, aMaxShift = 0, aToggle = l_toggle}) : transPArray (l_type, l_rank, l_toggle) ps

transPStencil :: (PType, Int, Int) -> [PName] -> [(PName, PStencil)]
transPStencil (l_type, l_rank, l_toggle) [] = []
transPStencil (l_type, l_rank, l_toggle) (p:ps) = (p, PStencil {sName = p, sType = l_type, sRank = l_rank, sToggle = l_toggle, sArrayInUse = [], sRegBound = False}) : transPStencil (l_type, l_rank, l_toggle) ps

transURange :: [([PName], PName, [DimExpr])] -> [(PName, PRange)]
transURange [] = []
transURange (p:ps) = (l_name, PRange {rName = l_name, rFirst = l_first, rLast = l_last, rStride = DimINT 1}) : transURange ps
    where l_name = pSecond p
          l_first = head $ pThird p
          l_last = head . tail $ pThird p


