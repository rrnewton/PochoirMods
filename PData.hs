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
data PMode = PHelp | PIter | POptPointer | PPointer | PTypeShadow | PInterior | PMacroShadow | PError deriving (Show, Eq)
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
    sToggle :: Int,
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
          | BExprVAR String Expr
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
          | DEXPR [PName] PType [Expr]
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
    show (BExprVAR a e) = a ++ "[" ++ show e ++ "]"
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
    show (DEXPR qs declType es) = intercalate " " qs ++ " " ++ 
        (intercalate ", " $ map show es) ++ ";"
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
                  showForExpr (DEXPR qs declType expr) = intercalate " " qs ++ 
                                                         " " ++ show expr
    showList [] = showString ""
    showList (x:xs) = showString breakline . shows x . showList xs

instance Show PShift where
    show (PShift n) = show n
    showList [] = showString ""
    showList (x:xs) = showChar '{' . shows x . showl xs
                        where showl [] = showChar '}'
                              showl (x:xs) = showString ", " . shows x . showl xs


