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
import PData
import qualified Data.Map as Map

pParser :: GenParser Char ParserState String
pParser = do tokens0 <- many $ pToken
             eof
             -- start a second pass!
             setInput $ concat tokens0
             tokens1 <- many pToken1
             return $ concat tokens1

pToken :: GenParser Char ParserState String
pToken = 
        do reserved "#define"
           l_name <- identifier
           pMacroValue l_name
    <|> do reserved "Pochoir_SArray"
           (l_type, l_rank) <- angles pDeclStatic <?> "Pochoir_SArray static parameter"
           l_arrayDecl <- commaSep1 pDeclDynamic
           {- use char ';'; eol; instead of 'semi' to preserve the relative position
            - in our output
            -}
           semi
           updateState $ updatePArray $ transPArray (l_type, l_rank) l_arrayDecl
           return ("Pochoir_SArray <" ++ show l_type ++ ", " ++ show l_rank ++ "> " ++ pShowArrayDynamicDecl l_arrayDecl ++ ";")
    <|> do reserved "Pochoir_Stencil"
           (l_type, l_rank) <- angles pDeclStatic <?> "Pochoir_Stencil static parameters"
           l_stencils <- commaSep1 identifier 
           semi
           updateState $ updatePStencil $ transPStencil (l_type, l_rank) l_stencils
           return (breakline ++ "Pochoir_Stencil <" ++ show l_type ++ ", " ++ show l_rank ++ "> " ++ pShowListIdentifiers l_stencils ++ ";")
    <|> do reserved "Pochoir_Shape_info"
           l_rank <- angles pDeclStaticNum
           l_name <- identifier
           l_len <- brackets pDeclStaticNum
           reservedOp "="
           l_shapes <- braces (commaSep1 ppShape)
           semi
           updateState $ updatePShape (l_name, l_rank, l_len, l_shapes)
           return (breakline ++ "Pochoir_Shape_info <" ++ show l_rank ++ "> " ++ l_name ++ " [" ++ show l_len ++ "] = " ++ pShowShapes l_shapes ++ ";")
    <|> do reserved "Pochoir_uRange"
           l_rangeDecl <- commaSep1 pDeclDynamic
           {- use char ';'; eol; instead of 'semi' to preserve the relative position
            - in our output
            -}
           semi
           updateState $ updatePRange $ transURange l_rangeDecl
           return (breakline ++ "Pochoir_uRange " ++ pShowArrayDynamicDecl l_rangeDecl ++ ";")
    <|> do reserved "Pochoir_kernel_1D"
           pPochoirKernel
    <|> do reserved "Pochoir_kernel_2D"
           pPochoirKernel
    <|> do reserved "Pochoir_kernel_3D"
           pPochoirKernel
    <|> do l_id <- try (pIdentifier)
           l_state <- getState
           case Map.lookup l_id $ pStencil l_state of
               Nothing -> return (l_id)
               Just l_stencil -> ppStencil l_id l_stencil l_state
    <|> do try (string "/*")
           str <- manyTill anyChar (try $ string "*/")
           -- return ("/* comment */")
           return ("/*" ++ str ++ "*/")
    <|> do try (string "//")
           str <- manyTill anyChar (try $ eol)
           -- return ("// comment\n")
           return ("//" ++ str ++ "\n")
    <|> do ch <- anyChar
           return [ch]
    <?> "line"

pPochoirKernel :: GenParser Char ParserState String
pPochoirKernel = 
    do  l_kernel_params <- parens $ commaSep1 identifier           
        exprStmts <- manyTill pStatement (try $ reserved "Pochoir_kernel_end")
        l_state <- getState
        let l_iters = getIterStmts exprStmts $ pArray l_state
        let l_revIters = transIterN 0 l_iters
        let l_kernel = PKernel { kName = head l_kernel_params, 
                         kParams = tail l_kernel_params,
                         kStmt = exprStmts, kIter = l_revIters }
        updateState $ updatePKernel l_kernel
        return (pShowKernel (kName l_kernel) l_kernel)

transIterN :: Int -> [Iter] -> [Iter]
transIterN _ [] = []
transIterN n ((name, array, dim):is) = (name ++ show n, array, dim) : (transIterN (n+1) is)

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

updatePKernel :: PKernel -> ParserState -> ParserState
updatePKernel l_kernel parserState =
    parserState { pKernel = Map.insert (kName l_kernel) l_kernel $ pKernel parserState }

updateObase :: Bool -> ParserState -> ParserState
updateObase ob parserState = parserState { pObase = ob }

updatePState :: PState -> ParserState -> ParserState
updatePState pstate parserState 
    | (pstate == PochoirBegin && pState parserState == Unrelated) = parserState { pState = PochoirBegin }
    | (pstate == PochoirEnd && pState parserState == PochoirBegin) = parserState { pState = Unrelated }
    | ((pstate == PochoirMacro || pstate == PochoirDeclArray || pstate == PochoirDeclRange) 
            && pState parserState == Unrelated) = parserState { pState = pstate }
    | pstate == Unrelated = parserState { pState = Unrelated }
    | otherwise = parserState { pState = PochoirError }

{-
 - the 'ptr' won't pass the type checking stage,
 - because 'ptr' is not a (visible) constructor field name!!
 
updatePTable :: [a] -> b -> ParserState -> ParserState
updatePTable ppvalue ptr parserState = 
    parserState { ptr = ppvalue ++ (ptr parserState) }
-}
updatePMacro :: (PName, PValue) -> ParserState -> ParserState
updatePMacro (l_name, l_value) parserState =
    parserState { pMacro = Map.insert l_name l_value (pMacro parserState) }

updatePShape :: (PName, Int, PValue, [[Int]]) -> ParserState -> ParserState
updatePShape (l_name, l_rank, l_len, l_shape) parserState =
    let l_pShape = PShape {shapeName = l_name, shapeRank = l_rank, shapeLen = l_len, shape = l_shape}
    in parserState { pShape = Map.insert l_name l_pShape (pShape parserState) }

updatePArray :: [(PName, PArray)] -> ParserState -> ParserState
updatePArray [] parserState = parserState
updatePArray pL@(p:ps) parserState =
    parserState { pArray = foldr pMapInsert (pArray parserState) pL }

updatePStencil :: [(PName, PStencil)] -> ParserState -> ParserState
updatePStencil [] parserState = parserState
updatePStencil pL@(p:ps) parserState =
    parserState { pStencil = foldr pMapInsert (pStencil parserState) pL }

updatePRange :: [(PName, PRange)] -> ParserState -> ParserState
updatePRange [] parserState = parserState
updatePRange pL@(p:ps) parserState =
    parserState { pRange = foldr pMapInsert (pRange parserState) pL }

pMapInsert :: (Ord k) => (k, a) -> Map.Map k a -> Map.Map k a
pMapInsert (l_key, l_value) l_map = Map.insert l_key l_value l_map

transPArray :: (PType, Int) -> [(PName, [Int])] -> [(PName, PArray)]
transPArray (l_type, l_rank) [] = []
transPArray (l_type, l_rank) (p:ps) =
    (l_name, PArray {aName = l_name, aType = l_type, aRank = l_rank, aDims = l_dims, aMaxShift = 0, aToggle = 0}) : transPArray (l_type, l_rank) ps
    where l_name = fst p
          l_dims = snd p

transPStencil :: (PType, Int) -> [PName] -> [(PName, PStencil)]
transPStencil (l_type, l_rank) [] = []
transPStencil (l_type, l_rank) (p:ps) = (p, PStencil {sName = p, sType = l_type, sRank = l_rank, sArrayInUse = []}) : transPStencil (l_type, l_rank) ps

transURange :: [(PName, [Int])] -> [(PName, PRange)]
transURange [] = []
transURange (p:ps) = (l_name, PRange {rName = l_name, rFirst = l_first, rLast = l_last, rStride = 1}) : transURange ps
    where l_name = fst p
          l_first = head $ snd p
          l_last = head . tail $ snd p


