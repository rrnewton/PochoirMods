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

module PData where

import Text.ParserCombinators.Parsec
import Control.Monad

import Data.Char
import Data.List
import qualified Data.Map as Map

type PName = String
type PValue = Int
type Uop = String 
type Bop = String
breakline :: String
breakline = "\n\t"

stripWhite :: String -> String
stripWhite l = dropWhile isSpace l

-- We use newtype just because we want to manually derive a Show instance of PShift
newtype PShift = PShift Int deriving (Eq, Ord)
data RegionT = Periodic | Nonperiodic | UnknownRegionT deriving Show
data PType = PInt | PDouble | PFloat | PBool | PUnknownType deriving Eq
data PState = PochoirBegin | PochoirEnd | PochoirMacro | PochoirDeclArray | PochoirDeclRange | PochoirError | Unrelated deriving (Show, Eq)
data PMode = PIter | PPointer | PTypeShadow | PInterior | PMacroShadow | PError deriving (Show, Eq)
data PMacro = PMacro {
    mName :: PName,
    mValue :: PValue
} deriving Show
data PArray = PArray {
    aName :: PName,
    aType :: PType,
    aRank :: Int,
    aMaxShift :: Int,
    aToggle :: Int,
    aDims :: [DimExpr]
} deriving (Show, Eq)
data PStencil = PStencil {
    sName :: PName,
    sType :: PType,
    sRank :: Int,
    sArrayInUse :: [PArray],
    sRegBound :: Bool
} deriving Show
data PShape = PShape {
    shapeName :: PName,
    shapeRank :: Int,
    shapeLen :: Int,
    shape :: [[Int]]
} deriving Show
data PRange = PRange {
    rName :: PName,
    rFirst :: DimExpr,
    rLast :: DimExpr,
    rStride :: DimExpr 
} deriving Show 
-- (NameOfIter, arrayInUse, correspondingDimExpr)
type Iter = (String, PArray, [DimExpr])
data PKernel = PKernel {
    kName :: PName,
    kParams :: [PName],
    kStmt :: [Stmt],
    kIter :: [Iter]
} deriving Show

data ParserState = ParserState {
    pMode  :: PMode,
    pState :: PState, 
    pMacro :: Map.Map PName PValue, 
    pArray :: Map.Map PName PArray,
    pStencil :: Map.Map PName PStencil,
    pRange :: Map.Map PName PRange,
    pShape :: Map.Map PName PShape,
    pKernel :: Map.Map PName PKernel
} deriving Show

data Expr = VAR String 
          -- PVAR : p(a, b, ...)
          | PVAR String [DimExpr]
          -- BVAR : v[a]
          | BVAR String DimExpr
          -- Uno is prefix unary operator
          | Uno Uop Expr
          -- PostUno is postfix unary operator
          | PostUno Uop Expr
          | Duo Bop Expr Expr
          | INT Int
          | FLOAT Double
          | BOOL String
          | PARENS Expr
          deriving Eq

data Stmt = BRACES [Stmt]
          | EXPR Expr
          | DEXPR PType [Expr]
          | IF Expr Stmt Stmt
          | SWITCH Expr [Stmt]
          | CASE PValue [Stmt]
          | DEFAULT [Stmt]
          | NOP
          | BREAK
          | DO Expr [Stmt]
          | WHILE Expr Stmt
          | FOR [[Stmt]] Stmt
          | UNKNOWN String
          deriving Eq

data DimExpr = DimVAR String 
          | DimDuo Bop DimExpr DimExpr
          | DimParen DimExpr
          | DimINT Int
          deriving Eq

instance Show PType where
    show PInt = "int"
    show PDouble = "double"
    show PFloat = "float"
    show PBool = "bool"
    show PUnknownType = "UnknownType"

instance Show Expr where
    show (VAR str) = str
    show (PVAR a xList@(x:xs)) = a ++ "(" ++ showList xList "" ++ ")"
    show (BVAR a x) = a ++ "[" ++ show x ++ "]"
    show (Uno uop expr) = uop ++ show expr 
    show (PostUno uop expr) = show expr ++ uop
    show (Duo bop lexpr rexpr) = show lexpr ++ " " ++ bop ++ " " ++ show rexpr
    show (PARENS expr) = "(" ++ show expr ++ ")"
    show (INT n) = show n
    show (FLOAT n) = show n
    show (BOOL b) = b
    showList [] = showString ""
    showList (x:xs) = showString breakline . shows x . showChar ';' . showList xs

instance Show DimExpr where
    show (DimVAR str) = str
    show (DimDuo bop lexpr rexpr) = show lexpr ++ " " ++ bop ++ " " ++ show rexpr
    show (DimParen e) = "(" ++ show e ++ ")"
    show (DimINT n) = show n
    showList (x:xs) = shows x . showl xs
                      where showl [] = showString "" 
                            showl (x:xs) = showString ", " . shows x . showl xs

instance Show Stmt where
    show NOP = ""
    show (UNKNOWN stmt) = stmt ++ "\t/* Unrecognized! */" 
    show BREAK = "break;"
    show (EXPR expr) = show expr ++ ";"
    show (DEXPR declType es) = show declType ++ " " ++ 
                                (concat $ intersperse ", " $ map show es) ++ ";"
    show (BRACES tL@(t:ts)) = "{" ++ showList tL "" ++ breakline ++ "}"
    show (IF expr l_stmt NOP) = "if " ++ show expr ++ 
        breakline ++ show l_stmt 
    show (IF expr l_stmt r_stmt) = "if " ++ show expr ++ 
        breakline ++ show l_stmt ++ 
        breakline ++ "else " ++ show r_stmt
    show (SWITCH expr tL@(t:ts)) = "switch " ++ show expr ++ "{" ++
        showList tL "" ++ breakline ++ "} /* end of switch */" ++ breakline
    show (CASE l_value tL@(t:ts)) = "case " ++ show l_value ++ " : " ++
        showList tL ""
    show (WHILE expr stmt) = "while " ++ show expr ++ 
        show stmt ++ breakline ++ "/* end of while */" ++ breakline
    show (DO expr tL@(t:ts)) = "do " ++ "{" ++
        showList tL "" ++ breakline ++ "} while " ++ show expr ++ ";" ++ breakline
    show (DEFAULT tL@(t:ts)) = "default :" ++ showList tL "" 
    show (FOR ttL@(t:ts) l_stmt) = "for " ++ showForListList ttL ++
        breakline ++ show l_stmt
            where showForListList (t:ts) = "(" ++ showForList t ++ showForListListL ts
                  showForListListL [] = ")"
                  showForListListL (t:ts) = "; " ++ showForList t ++ showForListListL ts
                  showForList (t:ts) = showForExpr t ++ showForListL ts
                  showForListL [] = ""
                  showForListL (t:ts) = ", " ++ showForExpr t ++ showForListL ts
                  showForExpr NOP = ""
                  showForExpr (EXPR expr) = show expr 
                  showForExpr (DEXPR declType expr) = show declType ++ " " ++ show expr
    showList [] = showString ""
    showList (x:xs) = showString breakline . shows x . showList xs

instance Show PShift where
    show (PShift n) = show n
    showList [] = showString ""
    showList (x:xs) = showChar '{' . shows x . showl xs
                        where showl [] = showChar '}'
                              showl (x:xs) = showString ", " . shows x . showl xs

simplifyDimExpr :: DimExpr -> DimExpr
simplifyDimExpr de = 
    let newDimExpr = simplifyDimExprItem de
    in  if newDimExpr == de then newDimExpr
                            else simplifyDimExpr newDimExpr

simplifyDimExprItem :: DimExpr -> DimExpr
simplifyDimExprItem (DimVAR v) = DimVAR v
simplifyDimExprItem (DimINT n) = DimINT n
simplifyDimExprItem (DimDuo "-" (DimINT 0) (DimINT n)) = DimINT (-n)
simplifyDimExprItem (DimDuo bop e1 e2)
    | bop == "+" && e1 == DimINT 0 = e2
    | bop == "+" && e2 == DimINT 0 = e1
    | bop == "-" && e2 == DimINT 0 = e1
    | bop == "*" && e1 == DimINT 0 = DimINT 0
    | bop == "*" && e2 == DimINT 0 = DimINT 0
    | bop == "*" && e1 == DimINT 1 = e2
    | bop == "*" && e2 == DimINT 1 = e1
    | otherwise = DimDuo bop (simplifyDimExprItem e1) (simplifyDimExprItem e2)
simplifyDimExprItem (DimParen e) = DimParen (simplifyDimExprItem e)

getFromStmts :: (PArray -> Expr -> [Iter]) -> Map.Map PName PArray -> [Stmt] -> [Iter]
getFromStmts l_action _ [] = []
getFromStmts l_action l_arrayMap l_stmts@(a:as) = 
    let i1 = getFromStmt a 
        i2 = getFromStmts l_action l_arrayMap as 
    in  union i1 i2
    where getFromStmt (BRACES stmts) = getFromStmts l_action l_arrayMap stmts 
          getFromStmt (EXPR e) = getFromExpr e
          getFromStmt (DEXPR t es) = concat $ map getFromExpr es
          getFromStmt (IF e s1 s2) = 
              let iter1 = getFromExpr e 
                  iter2 = getFromStmt s1 
                  iter3 = getFromStmt s2 
              in  union iter1 (union iter2 iter3)
          getFromStmt (SWITCH e stmts) = 
              let iter1 = getFromExpr e 
                  iter2 = getFromStmts l_action l_arrayMap stmts
              in  union iter1 iter2
          getFromStmt (CASE v stmts) = getFromStmts l_action l_arrayMap stmts
          getFromStmt (DEFAULT stmts) = getFromStmts l_action l_arrayMap stmts
          getFromStmt NOP = []
          getFromStmt BREAK = []
          getFromStmt (DO e stmts) = 
              let iter1 = getFromExpr e 
                  iter2 = getFromStmts l_action l_arrayMap stmts
              in  union iter1 iter2
          getFromStmt (WHILE e stmt) = 
              let iter1 = getFromExpr e 
                  iter2 = getFromStmt stmt
              in  union iter1 iter2
          getFromStmt (FOR sL s) = 
              let iter1 = getFromStmt s 
                  iter2 = concat $ map (getFromStmts l_action l_arrayMap) sL
              in  union iter1 iter2
          getFromStmt (UNKNOWN s) = []
          getFromExpr (VAR v) = []
          getFromExpr (BVAR v dim) = []
          getFromExpr (PVAR v dL) = 
              case Map.lookup v l_arrayMap of
                   Nothing -> []
                   Just arrayInUse -> l_action arrayInUse (PVAR v dL)
          getFromExpr (Uno uop e) = getFromExpr e
          getFromExpr (PostUno uop e) = getFromExpr e
          getFromExpr (Duo bop e1 e2) = 
              let iter1 = getFromExpr e1 
                  iter2 = getFromExpr e2
              in  (union iter1 iter2)
          getFromExpr (PARENS e) = getFromExpr e
          getFromExpr _ = []

transStmts :: [Stmt] -> (Expr -> Expr) -> [Stmt]
transStmts [] _ = []
transStmts l_stmts@(a:as) l_action = transStmt a : transStmts as l_action
    where transStmt (BRACES stmts) = BRACES $ transStmts stmts l_action
          transStmt (EXPR e) = EXPR $ transExpr e
          transStmt (DEXPR t es) = DEXPR t $ map transExpr es 
          transStmt (IF e s1 s2) = IF (transExpr e) 
                                              (transStmt s1) 
                                              (transStmt s2) 
          transStmt (SWITCH e stmts) = SWITCH (transExpr e) 
                                                  (transStmts stmts l_action)
          transStmt (CASE v stmts) = CASE v $ transStmts stmts l_action
          transStmt (DEFAULT stmts) = DEFAULT $ transStmts stmts l_action
          transStmt NOP =  NOP
          transStmt BREAK =  BREAK
          transStmt (DO e stmts) = DO (transExpr e) (transStmts stmts l_action)
          transStmt (WHILE e stmt) = WHILE (transExpr e)
                                                (transStmt stmt)
          transStmt (FOR sL s) = FOR (map (flip transStmts l_action) sL)
                                         (transStmt s) 
          transStmt (UNKNOWN s) = UNKNOWN s 
          transExpr (VAR v) =  VAR v
          -- if it's in the form of BVAR, then the user must have already done some
          -- manual transformation on its source, we just leave them untouched!
          transExpr (BVAR v dim) = BVAR v dim
          transExpr (PVAR v dL) = l_action (PVAR v dL)
          transExpr (Uno uop e) = Uno uop $ transExpr e
          transExpr (PostUno uop e) = PostUno uop $ transExpr e
          transExpr (Duo bop e1 e2) = Duo bop (transExpr e1) (transExpr e2)
          transExpr (PARENS e) = PARENS $ transExpr e
          transExpr (INT n) = (INT n)
          transExpr (FLOAT f) = (FLOAT f)
          transExpr (BOOL b) = (BOOL b)

pIterLookup :: (String, [DimExpr]) -> [Iter] -> Maybe String
pIterLookup (v, dL) [] = Nothing
pIterLookup (v, dL) ((iterName, arrayInUse, dim):is) 
    | v == aName arrayInUse && dL == dim = Just iterName
    | otherwise = pIterLookup (v, dL) is

pShowShadowArrayInUse :: [PArray] -> String
pShowShadowArrayInUse [] = ""
pShowShadowArrayInUse aL@(a:as) =
    pShowShadowArrayItem a ++ pShowShadowArrayInUse as
    where pShowShadowArrayItem a = 
            let l_type = aType a 
                l_rank = aRank a
                l_toggle = aToggle a
                l_name = aName a
                pShowShadowHeader (l_type, l_rank, l_toggle) = "interior_shadow<" ++
                    show l_type ++ ", " ++ show l_rank ++ "> "
            in breakline ++ pShowShadowHeader (l_type, l_rank, l_toggle) ++ 
                l_name ++ "_shadow(" ++ l_name ++ ");" ++
                breakline ++ pShowShadowHeader (l_type, l_rank, l_toggle) ++
                l_name ++ "(" ++ l_name ++ "_shadow);" ++ breakline

pDefMacroArrayInUse :: [PArray] -> [PName] -> String
pDefMacroArrayInUse [] _ = ""
pDefMacroArrayInUse (a:as) pL = pDefMacroShadowItem a pL ++ pDefMacroArrayInUse as pL
    where pDefMacroShadowItem a pL = 
            let l_arrayName = aName a
                l_arrayInteriorName = l_arrayName ++ ".interior"
            in  "#define " ++ pShowArrayTerm l_arrayName pL ++ " " ++
                pShowArrayTerm l_arrayInteriorName pL ++ breakline

pShowArrayTerm :: PName -> [PName] -> String
pShowArrayTerm a pL = a ++ "(" ++ pShowListIdentifiers pL ++ ")"

pUndefMacroArrayInUse :: [PArray] -> [PName] -> String
pUndefMacroArrayInUse [] _ = ""
pUndefMacroArrayInUse (a:as) pL = pUndefMacroShadowItem a pL ++ pUndefMacroArrayInUse as pL
    where pUndefMacroShadowItem a pL = 
            let l_arrayName = aName a
            in  "#undef " ++ pShowArrayTerm l_arrayName pL ++ breakline

pShowKernel :: String -> PKernel -> String
pShowKernel l_name l_kernel = "Pochoir_kernel_" ++ show dim ++ "D(" ++ l_name ++ ", " ++
    pShowKernelParams (kParams l_kernel) ++ ")" ++ show (kStmt l_kernel) ++
    breakline ++ "Pochoir_kernel_end" ++ breakline
        where dim = length (kParams l_kernel) - 1

pShowKernelParams :: [String] -> String
pShowKernelParams l_kernel_params = concat $ intersperse ", " l_kernel_params

pShowObaseKernel :: String -> PKernel -> String
pShowObaseKernel l_name l_kernel = 
    let l_rank = length (kParams l_kernel) - 1
        l_iter = kIter l_kernel
        l_array = unionArrayIter l_iter
        l_t = head $ kParams l_kernel
    in  breakline ++ "Pochoir_obase_fn_" ++ show l_rank ++ 
        "D(" ++ l_name ++ ", t0, t1, grid)" ++
        breakline ++ "grid_info<" ++ show l_rank ++ "> l_grid = grid;" ++
        pShowIters l_iter ++ breakline ++ "int " ++ 
        concat (intersperse ", " (map (getArrayGaps (l_rank-1)) l_array)) ++ ";" ++
        breakline ++ pShowStrides l_rank l_array ++ breakline ++
        "for (int " ++ l_t ++ " = t0; " ++ l_t ++ " < t1; ++" ++ l_t ++ ") { " ++ 
        pShowIterSet l_iter (kParams l_kernel)++
        breakline ++ pShowObaseForHeader l_rank l_iter (tail $ kParams l_kernel) ++
        breakline ++ pShowObaseStmt l_kernel ++ breakline ++ pShowObaseForTail l_rank ++
        pShowObaseTail l_rank ++ breakline ++ "Pochoir_kernel_end\n"

pShowPointerKernel :: String -> PKernel -> String
pShowPointerKernel l_name l_kernel = 
    let l_rank = length (kParams l_kernel) - 1
        l_iter = kIter l_kernel
        l_array = unionArrayIter l_iter
        l_t = head $ kParams l_kernel
    in  breakline ++ "Pochoir_obase_fn_" ++ show l_rank ++ 
        "D(" ++ l_name ++ ", t0, t1, grid)" ++
        breakline ++ "grid_info<" ++ show l_rank ++ "> l_grid = grid;" ++
        pShowPointers l_iter ++ breakline ++ 
        pShowArrayInfo l_array ++ 
        "int " ++ 
        concat (intersperse ", " (map (getArrayGaps (l_rank-1)) l_array)) ++ ";" ++
        breakline ++ pShowStrides l_rank l_array ++ breakline ++
        "for (int " ++ l_t ++ " = t0; " ++ l_t ++ " < t1; ++" ++ l_t ++ ") { " ++ 
        pShowPointerSet l_iter (kParams l_kernel)++
        breakline ++ pShowPointerForHeader l_rank l_iter (tail $ kParams l_kernel) ++
        breakline ++ pShowPointerStmt l_kernel ++ breakline ++ pShowObaseForTail l_rank ++
        pShowObaseTail l_rank ++ breakline ++ "Pochoir_kernel_end\n"

pShowPragma :: String
pShowPragma = "#pragma ivdep"

pShowArrayInfo :: [PArray] -> String
pShowArrayInfo arrayInUse = foldr pShowArrayInfoItem "" arrayInUse
    where pShowArrayInfoItem l_arrayItem str =
            let l_type = aType l_arrayItem
                l_name = aName l_arrayItem
            in  str ++ breakline ++ show l_type ++ " * " ++ l_name ++ "_base"  ++ 
                " = " ++ l_name ++ ".data();" ++ breakline ++
                "const int " ++ "l_" ++ l_name ++ "_total_size = " ++ l_name ++
                ".total_size();" ++ breakline

pShowPointers :: [Iter] -> String
pShowPointers iL@(i:is) = foldr pShowPointer "" iL
    where pShowPointer (nameIter, arrayInUse, dL) str =
                str ++ breakline ++ (show $ aType arrayInUse) ++ " * " ++ nameIter ++ ";"

pShowPointerStmt :: PKernel -> String
pShowPointerStmt l_kernel = 
    let oldStmts = kStmt l_kernel
        l_iter = kIter l_kernel
        obaseStmts = transStmts oldStmts $ transPointer l_iter
    in show obaseStmts

pShowObaseStmt :: PKernel -> String
pShowObaseStmt l_kernel = 
    let oldStmts = kStmt l_kernel
        l_iter = kIter l_kernel
        obaseStmts = transStmts oldStmts $ transIter l_iter
    in show obaseStmts

transPointer :: [Iter] -> Expr -> Expr
transPointer l_iters (PVAR v dL) =
    case pPointerLookup (v, dL) l_iters of
        Nothing -> PVAR v dL
        Just (iterName, arrayInUse, des) -> 
            BVAR iterName de
                where de = simplifyDimExpr naive_de
                      naive_de = foldr plusCombDimExpr x $ zipWith mulDimExpr strideL $ tail $ excludeDimExpr dL des 
                      strideL = pGetArrayStrideList (aRank arrayInUse) (aName arrayInUse)
                      x = (DimINT 0)
transPointer l_iters e = e

plusCombDimExpr :: DimExpr -> DimExpr -> DimExpr
plusCombDimExpr e1 e2 = DimDuo "+" e1 e2

mulDimExpr :: String -> DimExpr -> DimExpr
mulDimExpr stride dim = DimDuo "*" (DimVAR stride) dim

excludeDimExpr :: [DimExpr] -> [DimExpr] -> [DimExpr]
excludeDimExpr [] [] = []
excludeDimExpr (d:ds) (r:rs) = (excludeDimExprItem d r):(excludeDimExpr ds rs)
    where excludeDimExprItem (DimVAR v) r = if (DimVAR v) == r then DimINT 0 else (DimVAR v)
          excludeDimExprItem (DimDuo bop e1 e2) r 
            | e1 == r = DimParen (DimDuo bop (DimINT 0) e2)
            | e2 == r = DimParen (DimDuo bop e1 (DimINT 0))
            | otherwise = (DimDuo bop (excludeDimExprItem e1 r) (excludeDimExprItem e2 r))
          excludeDimExprItem (DimINT n) r = if (DimINT n) == r then DimINT 0 else (DimINT n)

pPointerLookup :: (PName, [DimExpr]) -> [Iter] -> Maybe Iter
pPointerLookup (v, dL) [] = Nothing
pPointerLookup (v, dL) ((iterName, arrayInUse, dL'):is)
    | v == aName arrayInUse && head dL == head dL' = Just (iterName, arrayInUse, dL')
    | otherwise = pPointerLookup (v, dL) is

transIter :: [Iter] -> Expr -> Expr
transIter l_iters (PVAR v dL) =
    case pIterLookup (v, dL) l_iters of
        Nothing -> PVAR v dL
        Just iterName -> VAR iterName
transIter l_iters e = e

pShowIterSet :: [Iter] -> [PName] -> String
pShowIterSet iL@(i:is) l_kernelParams = concat $ map pShowIterSetTerm iL
    where pShowIterSetTerm (name, array, dim) = 
            breakline ++ name ++ ".set(" ++ show (pShowTransDim dim l_kernelParams) ++ ");" 

-- PName : list of kernel parameters
pShowPointerSet :: [Iter] -> [PName] -> String
pShowPointerSet iL@(i:is) l_kernelParams = concat $ map pShowPointerSetTerm iL
    where pShowPointerSetTerm (iterName, array, dim) = 
            let l_arrayName = aName array
                l_arrayBaseName = l_arrayName ++ "_base"
                l_arrayTotalSize = "l_" ++ l_arrayName ++ "_total_size"
                l_arrayStrideList = 
                    pGetArrayStrideList (length l_kernelParams - 1) l_arrayName
                l_transDimList = tail $ pShowTransDim dim l_kernelParams
                l_arraySpaceOffset = 
                    concat $ intersperse " + " $ zipWith pCombineDim l_transDimList l_arrayStrideList
                l_arrayTimeOffset = (pGetTimeOffset (aToggle array - 1) (head dim)) ++ 
                                    " * " ++ l_arrayTotalSize
            in  breakline ++ iterName ++ " = " ++ l_arrayBaseName ++ " + " ++ 
                l_arrayTimeOffset ++ " + " ++ l_arraySpaceOffset ++ ";" 

pGetTimeOffset :: Int -> DimExpr -> String
pGetTimeOffset toggle tDim = "((" ++ show tDim ++ ")" ++ " & " ++ show toggle ++ ")"

pCombineDim :: DimExpr -> String -> String
-- l_stride_pa_0 may NOT necessary be "1", 
-- plus that we have already set all strides to be of type "const int"
pCombineDim de stride = show de ++ " * " ++ stride

pGetArrayStrideList :: Int -> PName -> [String]
pGetArrayStrideList 1 arr = ["l_stride_" ++ arr ++ "_" ++ show 0]
pGetArrayStrideList n arr = ("l_stride_" ++ arr ++ "_" ++ show (n-1)):(pGetArrayStrideList (n-1) arr)

pShowTransDim :: [DimExpr] -> [PName] -> [DimExpr]
pShowTransDim (d:ds) (p:ps) = 
    let l_rank = length ds
    in  (d:(pTransDim 1 l_rank ds ps))

pTransDim :: Int -> Int -> [DimExpr] -> [PName] -> [DimExpr]
pTransDim n r [] _ = []
pTransDim n r dL@(d:ds) pL@(p:ps) = pTransDimTerm n r d p : pTransDim (n+1) r ds ps
    where pTransDimTerm n r (DimVAR v) p
              | v == p = DimVAR ("l_grid.x0[" ++ show (r-n) ++ "]")
              | otherwise = DimVAR v
          pTransDimTerm n r (DimINT i) p = DimINT i
          pTransDimTerm n r (DimDuo bop e1 e2) p = 
              DimDuo bop (pTransDimTerm n r e1 p) (pTransDimTerm n r e2 p)
        
pShowStrides :: Int -> [PArray] -> String
pShowStrides n aL = "const int " ++ getStrides n aL ++ ";\n"
    where getStrides n aL@(a:as) = concat . intersperse ", " $ concat $ map (getStride n) aL
          getStride 1 a = let r = 0 
                          in  ["l_stride_" ++ (aName a) ++ "_" ++ show r ++
                              " = " ++ (aName a) ++ ".stride(" ++ show r ++ ")"]
          getStride n a = let r = n-1
                          in  ["l_stride_" ++ (aName a) ++ "_" ++ show r ++
                              " = " ++ (aName a) ++ ".stride(" ++ show r ++ ")"] ++
                              getStride (n-1) a

pShowIters :: [Iter] -> String
pShowIters [] = ""
pShowIters ((l_name, l_array, l_dim):is) = 
    let l_type = aType l_array
        l_rank = aRank l_array
        l_arrayName = aName l_array
    in breakline ++ "Pochoir_Iterator<" ++ show l_type ++ ", " ++ show l_rank ++ "> " ++
       l_name ++ "(" ++ l_arrayName ++ ");" ++ pShowIters is   

unionArrayIter :: [Iter] -> [PArray]
unionArrayIter [] = []
unionArrayIter iL@(i:is) = union (getArrayItem i) (unionArrayIter is)
    where getArrayItem (_, a, _) = [a]

getArrayIter :: [Iter] -> [PName]
getArrayIter [] = []
getArrayIter iL@(i:is) = (getArrayItem i) ++ (getArrayIter is)
    where getArrayItem (_, a, _) = [aName a]

pShowObaseForTail :: Int -> String
pShowObaseForTail n 
    | n == 0 = "/* end for (sub-trapezoid) */ "
    | otherwise = "} " ++ pShowObaseForTail (n-1)

pShowObaseTail :: Int -> String
pShowObaseTail n = 
    breakline ++ "/* Adjust sub-trapezoid! */" ++
    breakline ++ "for (int i = 0; i < " ++ show n ++ "; ++i) {" ++ 
    breakline ++ "\tl_grid.x0[i] += l_grid.dx0[i]; l_grid.x1[i] += l_grid.dx1[i];" ++
    breakline ++ "}" ++
    breakline ++ "} /* end for t */"

-- pL is the parameter list of original user supplied computing kernel
pShowObaseForHeader :: Int -> [Iter] -> [PName] -> String
pShowObaseForHeader 1 iL pL = 
                           breakline ++ pShowForHeader 0 (unionArrayIter iL) pL ++ 
                           breakline ++ concat (
                                   intersperse (", " ++ breakline) 
                                        (map ((++) "++" . getIterName) iL)) ++ ") {"
pShowObaseForHeader n iL pL = 
                           breakline ++ pShowForHeader (n-1) (unionArrayIter iL) pL ++ 
                           breakline ++ concat (
                                   intersperse (", " ++ breakline) 
                                     (zipWith wrapIterInc
                                        (map (getArrayGap (n-1)) (getArrayIter iL))
                                        (map getIterName iL))) ++ 
                           ") {" ++ pShowObaseForHeader (n-1) iL pL
    where wrapIterInc gap iter = iter ++ ".inc(" ++ gap ++ ")"

-- pL is the parameter list of original user supplied computing kernel
pShowPointerForHeader :: Int -> [Iter] -> [PName] -> String
pShowPointerForHeader 1 iL pL = 
                           breakline ++ pShowPragma ++
                           breakline ++ pShowForHeader 0 (unionArrayIter iL) pL ++ 
                           breakline ++ concat (
                                   intersperse (", " ++ breakline) 
                                        (map ((++) "++" . getIterName) iL)) ++ ") {"
pShowPointerForHeader n iL pL = 
                           breakline ++ pShowForHeader (n-1) (unionArrayIter iL) pL ++ 
                           breakline ++ concat (
                                   intersperse (", " ++ breakline) 
                                     (zipWith wrapIterInc
                                        (map (getArrayGap (n-1)) (getArrayIter iL))
                                        (map getIterName iL))) ++ 
                           ") {" ++ pShowPointerForHeader (n-1) iL pL
    where wrapIterInc gap iter = iter ++ " += " ++ gap 

pShowForHeader :: Int -> [PArray] -> [PName] -> String
pShowForHeader i aL@(a:as) pL = 
    let len_pL = length pL
        idx = pL !! (len_pL - 1 - i)
        l_rank = show i
    in  adjustGap i aL ++ "for (int " ++ idx ++ 
        " = l_grid.x0[" ++ l_rank ++
        "]; " ++ idx ++ " < l_grid.x1[" ++ l_rank ++ "]; ++" ++ 
        idx ++ ","
                    
adjustGap :: Int -> [PArray] -> String
adjustGap i aL@(a:as) = 
    if i > 0 then pShowAdjustGap i aL
             else ""
    where pShowAdjustGap i [] = ""
          pShowAdjustGap i aL@(a:as) = concat $ map (pShowAdjustGapTerm i) aL
          pShowAdjustGapTerm i a = "gap_" ++ (aName a) ++ "_" ++ show i ++ " = " ++
                                   "l_stride_" ++ (aName a) ++ "_" ++ show i ++ 
                                   " + (l_grid.x0[" ++ show (i-1) ++ "] - l_grid.x1[" ++
                                   show (i-1) ++ "]) * l_stride_" ++ (aName a) ++ "_" ++ 
                                   show (i-1) ++ ";" ++ breakline
          
getIterName :: Iter -> String
getIterName (name, _, _) = name

getArrayGaps :: Int -> PArray -> String
getArrayGaps 0 array = getArrayGap 0 (aName array)
getArrayGaps n array = getArrayGap n (aName array) ++ ", " ++ getArrayGaps (n-1) array

getArrayGap :: Int -> PName -> String
getArrayGap n array = "gap_" ++ array ++ "_" ++ show n

pShowShapes :: [[Int]] -> String
pShowShapes [] = ""
pShowShapes aL@(a:as) = "{" ++ pShowShape a ++ pShowShapesL as
    where pShowShapesL [] = "}"
          pShowShapesL (x:xs) = ", " ++ pShowShape x ++ pShowShapesL xs

pShowShape :: [Int] -> String
pShowShape [] = ""
pShowShape aL@(a:as) = "{" ++ show a ++ pShowShapeL as
    where pShowShapeL [] = "}"
          pShowShapeL (x:xs) = ", " ++ show x ++ pShowShapeL xs

pShowListIdentifiers :: [PName] -> String
pShowListIdentifiers [] = ""
pShowListIdentifiers (n:ns) = n ++ pShowListIdentifiersL ns
    where pShowListIdentifiersL [] = ""
          pShowListIdentifiersL nL@(n:ns) = ", " ++ pShowListIdentifiers nL

pShowArrayDynamicDecl :: [(PName, [DimExpr])] -> String
pShowArrayDynamicDecl [] = ""
pShowArrayDynamicDecl (p:ps) = pShowArrayItem p ++  pShowArrayDynamicDeclL ps
    where pShowArrayDynamicDeclL [] = ""
          pShowArrayDynamicDeclL qL@(q:qs) = ", " ++ pShowArrayDynamicDecl qL

pShowArrayItem :: (PName, [DimExpr]) -> String
pShowArrayItem (a, bL@(b:bs)) = a ++ pShowArrayDim bL

pShowArrayDim :: [DimExpr] -> String
pShowArrayDim [] = ""
pShowArrayDim (c:cs) = "(" ++ show c ++ pShowArrayDimL cs
    where pShowArrayDimL [] = ")"
          pShowArrayDimL (d:ds) = ", " ++ show d ++ pShowArrayDimL ds


