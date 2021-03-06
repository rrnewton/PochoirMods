{-
 ----------------------------------------------------------------------------------
 -  Copyright (C) 2010-2011  Massachusetts Institute of Technology
 -  Copyright (C) 2010-2011  Yuan Tang <yuantang@csail.mit.edu>
 - 		                     Charles E. Leiserson <cel@mit.edu>
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

module PMainParser (pParser) where

import Text.ParserCombinators.Parsec

import Control.Monad

import PBasicParser
import PParser2
import PUtils
import PData
import PShow
-- import Text.Show
import qualified Data.Map as Map

-- The main parser, which ALSO is the main code generator.
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
    <|> try pParsePochoirArrayAsParam
    <|> try pParsePochoirStencil
    <|> try pParsePochoirStencilWithShape
    <|> try pParsePochoirStencilAsParam
    <|> try pParsePochoirStencilWithShapeAsParam
    <|> try pParsePochoirShapeInfo
    <|> try pParsePochoirDomain
    <|> try pParsePochoirKernel1D
    <|> try pParsePochoirKernel2D
    <|> try pParsePochoirKernel3D
    <|> try pParsePochoirAutoKernel
    <|> try pParsePochoirArrayMember
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
       (l_type, l_rank) <- angles $ try pDeclStatic
       l_arrayDecl <- commaSep1 pDeclDynamic
       l_delim <- pDelim 
       updateState $ updatePArray $ transPArray (l_type, l_rank) l_arrayDecl
       return (breakline ++ "/* Known*/ Pochoir_Array <" ++ show l_type ++ 
               ", " ++ show l_rank ++ "> " ++ 
               pShowDynamicDecl l_arrayDecl pShowArrayDim ++ l_delim)

pParsePochoirArrayAsParam :: GenParser Char ParserState String
pParsePochoirArrayAsParam =
    do reserved "Pochoir_Array"
       (l_type, l_rank) <- angles $ try pDeclStatic
       l_arrayDecl <- pDeclDynamic
       l_delim <- pDelim 
       updateState $ updatePArray $ transPArray (l_type, l_rank) [l_arrayDecl]
       return (breakline ++ "/* Known*/ Pochoir_Array <" ++ show l_type ++ 
               ", " ++ show l_rank ++ "> " ++ 
               pShowDynamicDecl [l_arrayDecl] pShowArrayDim ++ l_delim)

pParsePochoirStencil :: GenParser Char ParserState String
pParsePochoirStencil = 
    do reserved "Pochoir"
       l_rank <- angles exprDeclDim
       l_rawStencils <- commaSep1 pDeclPochoir
       l_delim <- pDelim
       l_state <- getState
       let l_stencils = map pSecond l_rawStencils
       let l_shapes = map pThird l_rawStencils
       let l_pShapes = map (getPShape l_state) l_shapes
       let l_toggles = map shapeToggle l_pShapes
       updateState $ updatePStencil $ transPStencil l_rank l_stencils l_pShapes
       return (breakline ++ "/* Known */ Pochoir <" ++ show l_rank ++ 
               "> " ++ pShowDynamicDecl l_rawStencils (showString "") ++ l_delim ++ 
               "/* toggles = " ++ show l_toggles ++ "*/")

pParsePochoirStencilWithShape :: GenParser Char ParserState String
pParsePochoirStencilWithShape = 
    do reserved "Pochoir"
       l_rank <- angles exprDeclDim
       l_rawStencils <- commaSep1 pDeclPochoirWithShape
       l_delim <- pDelim
       l_state <- getState
       let l_stencils = map pSecond l_rawStencils
       let l_pShapes = map pThird l_rawStencils 
       let l_toggles = map shapeToggle l_pShapes
       updateState $ updatePStencil $ transPStencil l_rank l_stencils l_pShapes
       return (breakline ++ "/* Known */ Pochoir <" ++ show l_rank ++ 
               "> " ++ pShowDynamicDecl l_rawStencils (pShowShapes . shape) ++ l_delim ++ 
               "/* toggles = " ++ (show $ map shapeToggle l_pShapes) ++ "*/")

pParsePochoirStencilAsParam :: GenParser Char ParserState String
pParsePochoirStencilAsParam = 
    do reserved "Pochoir"
       l_rank <- angles exprDeclDim
       l_rawStencil <- pDeclPochoir
       l_delim <- pDelim
       l_state <- getState
       let l_stencil = pSecond l_rawStencil
       let l_shape = pThird l_rawStencil
       let l_pShape = getPShape l_state l_shape
       let l_toggle = shapeToggle l_pShape
       updateState $ updatePStencil $ transPStencil l_rank [l_stencil] [l_pShape]
       return (breakline ++ "/* Known */ Pochoir <" ++ show l_rank ++ 
               "> " ++ pShowDynamicDecl [l_rawStencil] (showString "") ++ l_delim ++ 
               "/* toggles = " ++ show l_toggle ++ "*/")

pParsePochoirStencilWithShapeAsParam :: GenParser Char ParserState String
pParsePochoirStencilWithShapeAsParam = 
    do reserved "Pochoir"
       l_rank <- angles exprDeclDim
       l_rawStencil <- pDeclPochoirWithShape
       l_delim <- pDelim
       l_state <- getState
       let l_stencil = pSecond l_rawStencil
       let l_pShape = pThird l_rawStencil 
       let l_toggle = shapeToggle l_pShape
       updateState $ updatePStencil $ transPStencil l_rank [l_stencil] [l_pShape]
       return (breakline ++ "/* Known */ Pochoir <" ++ show l_rank ++ 
               "> " ++ pShowDynamicDecl [l_rawStencil] (pShowShapes . shape) ++ 
               l_delim ++ "/* toggles = " ++ (show $ shapeToggle l_pShape) ++ "*/")

pParsePochoirShapeInfo :: GenParser Char ParserState String
pParsePochoirShapeInfo = 
    do reserved "Pochoir_Shape"
       l_rank <- angles pDeclStaticNum
       l_name <- identifier
       brackets $ option 0 pDeclStaticNum
       reservedOp "="
       l_shapes <- braces (commaSep1 ppShape)
       semi
       let l_len = length l_shapes
       let l_toggle = getToggleFromShape l_shapes
       let l_slopes = getSlopesFromShape (l_toggle-1) l_shapes 
       updateState $ updatePShape (l_name, l_rank, l_len, l_toggle, l_slopes, l_shapes)
       return (breakline ++ "/* Known */ Pochoir_Shape <" ++ show l_rank ++ "> " ++ l_name ++ " [" ++ show l_len ++ "] = " ++ pShowShapes l_shapes ++ ";\n" ++ breakline ++ "/* toggle: " ++ show l_toggle ++ "; slopes: " ++ show l_slopes ++ " */\n")

pParsePochoirDomain :: GenParser Char ParserState String
pParsePochoirDomain =
    do reserved "Pochoir_Domain"
       l_rangeDecl <- commaSep1 pDeclDynamic
       semi
       updateState $ updatePRange $ transURange l_rangeDecl
       return (breakline ++ "Pochoir_Domain " ++ 
               pShowDynamicDecl l_rangeDecl pShowArrayDim ++ ";\n")

pParsePochoirKernel1D :: GenParser Char ParserState String
pParsePochoirKernel1D =
    do reserved "Pochoir_Kernel_1D"
       pPochoirKernel

pParsePochoirKernel2D :: GenParser Char ParserState String
pParsePochoirKernel2D =
    do reserved "Pochoir_Kernel_2D"
       pPochoirKernel

pParsePochoirKernel3D :: GenParser Char ParserState String
pParsePochoirKernel3D =
    do reserved "Pochoir_Kernel_3D"
       pPochoirKernel

pParsePochoirAutoKernel :: GenParser Char ParserState String
pParsePochoirAutoKernel =
    do reserved "auto"
       pPochoirAutoKernel

pParsePochoirArrayMember :: GenParser Char ParserState String
pParsePochoirArrayMember =
    do l_id <- try (pIdentifier)
       l_state <- getState
       try $ ppArray l_id l_state

pParsePochoirStencilMember :: GenParser Char ParserState String
pParsePochoirStencilMember =
    do l_id <- try (pIdentifier)
       l_state <- getState
       try $ ppStencil l_id l_state

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
        exprStmts <- manyTill pStatement (try $ reserved "Pochoir_Kernel_End")
        l_state <- getState
        let l_iters = getFromStmts (getPointer $ tail l_kernel_params) 
                                   (pArray l_state) exprStmts
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

transPArray :: (PType, Int) -> [([PName], PName, [DimExpr])] -> [(PName, PArray)]
transPArray (l_type, l_rank) [] = []
transPArray (l_type, l_rank) (p:ps) =
    let l_name = pSecond p
        l_dims = pThird p
    in  (l_name, PArray {aName = l_name, aType = l_type, aRank = l_rank, aDims = l_dims, aMaxShift = 0, aToggle = 0, aRegBound = False}) : transPArray (l_type, l_rank) ps

transPStencil :: Int -> [PName] -> [PShape] -> [(PName, PStencil)]
transPStencil l_rank [] _ = []
-- sToggle by default is two (2)
transPStencil l_rank (p:ps) (a:as) = (p, PStencil {sName = p, sRank = l_rank, sToggle = shapeToggle a, sArrayInUse = [], sShape = a, sRegBound = False}) : transPStencil l_rank ps as

transURange :: [([PName], PName, [DimExpr])] -> [(PName, PRange)]
transURange [] = []
transURange (p:ps) = (l_name, PRange {rName = l_name, rFirst = l_first, rLast = l_last, rStride = DimINT 1}) : transURange ps
    where l_name = pSecond p
          l_first = head $ pThird p
          l_last = head . tail $ pThird p


