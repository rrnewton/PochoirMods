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

module Main where

import System
import IO hiding (try) -- "try" is also defined in Parsec
import Data.List
--import System.FilePath
import System.Directory 
import System.Cmd (rawSystem)
import Data.Char (isSpace)
import qualified Data.Map as Map
import Text.ParserCombinators.Parsec (runParser)

import PData
import PMainParser

main :: IO ()
main = do args <- getArgs
          whilst (null args) $ do
             printUsage
             exitFailure
          let (inFile, inDir, mode, debug, showFile) 
                = parseArgs ("", "", PPointer, False, True) args
          whilst (mode == PHelp) $ do
             printUsage
             exitFailure
--          fileExist <- doesFileExist inFile
--          dirExist <- doesDirectoryExist inDir
          fileExist <- doesFileExist $ inDir ++ inFile
          whilst (not fileExist) $ do
             putStrLn (inDir ++ inFile ++ " doesn't exist!")
             exitFailure
          whilst (mode == PError) $ do
             putStrLn ("command line argument error" ++ concat args)
             exitFailure
          cilkHeaderPath <- catch (getEnv "CILK_HEADER_PATH")(\e -> return "EnvError")
          whilst (cilkHeaderPath == "EnvError") $ do
             putStrLn ("Environment variable CILK_HEADER_PATH is NOT set")
             exitFailure
          pochoirLibPath <- catch (getEnv "POCHOIR_LIB_PATH")(\e -> return "EnvError")
          whilst (pochoirLibPath == "EnvError") $ do
             putStrLn ("Environment variable POCHOIR_LIB_PATH is NOT set")
             exitFailure
          let envPath = ["-I" ++ cilkHeaderPath] ++ ["-I" ++ pochoirLibPath]
          let iccPPFile = inDir ++ getPPFile inFile
          let iccPPArgs = if debug == False
                then iccPPFlags ++ envPath ++ [inFile]
                else iccDebugPPFlags ++ envPath ++ [inFile] 
          -- a pass of icc preprocessing
          putStrLn ("inFile = " ++ inDir ++ inFile ++ 
                    "; icc preprocessed File = " ++ iccPPFile)
          rawSystem icc iccPPArgs
          -- a pass of pochoir compilation
          let outFile = rename inFile "_pochoir"
          putStrLn ("inFile = " ++ iccPPFile ++ "; Pochoir outFile = " ++ inDir ++ outFile)
          inh <- openFile iccPPFile ReadMode
          outh <- openFile outFile WriteMode
          pProcess mode inh outh
          hClose inh
          hClose outh
          -- a pass of compilation of user's original executable spec
          let objInFile  = getObjFile inDir inFile
          let iccInFileArgs = if debug == False 
                then objInFile ++ iccFlags ++ envPath ++ [inFile]
                else objInFile ++ iccDebugFlags ++ envPath ++ [inFile]
          putStrLn (icc ++ " " ++ intercalate " " iccInFileArgs)
          rawSystem icc iccInFileArgs
          -- a pass of compilation of pochoir optimized executable spec
          let objOutFile = getObjFile inDir outFile
          let iccOutFileArgs = if debug == False 
                then objOutFile ++ iccFlags ++ envPath ++ [outFile]
                else objOutFile ++ iccDebugFlags ++ envPath ++ [outFile]
          putStrLn (icc ++ " " ++ intercalate " " iccOutFileArgs)
          rawSystem icc iccOutFileArgs
          whilst (showFile == False) $ do
             removeFile outFile

whilst :: Bool -> IO () -> IO ()
whilst True action = action
whilst False action = return () 

rename :: String -> String -> String
rename fname pSuffix = name ++ pSuffix ++ ".cpp"
    where (name, suffix) = break ('.' ==) fname

getPPFile :: String -> String
getPPFile fname = name ++ ".i"
    where (name, suffix) = break ('.' ==) fname

getObjFile :: String -> String -> [String]
getObjFile dir fname = ["-o"] ++ [dir++name]
    where (name, suffix) = break ('.' ==) fname 

pInitState = ParserState { pMode = PPointer, pState = Unrelated, pMacro = Map.empty, pArray = Map.empty, pStencil = Map.empty, pShape = Map.empty, pRange = Map.empty, pKernel = Map.empty}

icc = "icc"

iccFlags = ["-O3", "-DNDEBUG", "-std=c++0x", "-Wall", "-Werror", "-ipo"]

iccPPFlags = ["-P", "-C", "-DNDEBUG", "-std=c++0x", "-Wall", "-Werror", "-ipo"]

iccDebugFlags = ["-DDEBUG", "-O0", "-g3", "-std=c++0x", "-include", "cilk_stub.h"]

iccDebugPPFlags = ["-P", "-C", "-DDEBUG", "-g3", "-std=c++0x", "-include", "cilk_stub.h"]

parseArgs :: (String, String, PMode, Bool, Bool) -> [String] -> (String, String, PMode, Bool, Bool)
parseArgs (inFile, inDir, mode, debug, showFile) aL 
    | elem "-help" aL =
        let l_mode = PHelp
        in  (inFile, inDir, l_mode, debug, showFile)
    | elem "-split-type-shadow" aL = 
        let l_mode = PTypeShadow
            aL' = delete "-split-type-shadow" aL
        in  parseArgs (inFile, inDir, l_mode, debug, showFile) aL'
    | elem "-split-pointer" aL =
        let l_mode = PPointer
            aL' = delete "-split-pointer" aL
        in  parseArgs (inFile, inDir, l_mode, debug, showFile) aL'
    | elem "-split-iter" aL =
        let l_mode = PIter
            aL' = delete "-split-iter" aL
        in  parseArgs (inFile, inDir, l_mode, debug, showFile) aL'
    | elem "-split-interior" aL =
        let l_mode = PInterior
            aL' = delete "-split-interior" aL
        in  parseArgs (inFile, inDir, l_mode, debug, showFile) aL'
    | elem "-split-macro-shadow" aL =
        let l_mode = PMacroShadow
            aL' = delete "-split-macro-shadow" aL
        in  parseArgs (inFile, inDir, l_mode, debug, showFile) aL'
    | elem "-showFile" aL =
        let l_showFile = True
            aL' = delete "-showFile" aL
        in  parseArgs (inFile, inDir, mode, debug, l_showFile) aL'
    | elem "-debug" aL =
        let l_debug = True
            aL' = delete "-debug" aL
        in  parseArgs (inFile, inDir, mode, l_debug, showFile) aL'
    | null aL == False =
        let (l_file, l_dir) = findCPP aL
        in  (l_file, l_dir, mode, debug, showFile)
    | otherwise = 
        let l_mode = PError
        in  (inFile, inDir, mode, debug, showFile)

findCPP :: [String] -> (String, String)
findCPP [] = ("", "")
findCPP (a:as)  
    | isSuffixOf ".cpp" a || isSuffixOf ".cxx" a = 
        let l_file = drop (1 + (pLast $ findIndices (== '/') a)) a
            l_dir  = take (1 + (pLast $ findIndices (== '/') a)) a
            pLast [] = -1
            pLast aL@(a:as) = last aL
        in  (l_file, l_dir)
    | otherwise = findCPP as

printUsage :: IO ()
printUsage = 
    do putStrLn ("Usage: ")
       putStrLn ("pochoir -split-type-shadow $filename : " ++ breakline ++ 
               "using type tricks to split the interior and boundary regions")
       putStrLn ("pochoir -split-macro-shadow $filename : " ++ breakline ++ 
               "using macro tricks to split the interior and boundary regions")
       putStrLn ("pochoir -split-iter $filename : " ++ breakline ++ 
               "split the interior and boundary region, and using iterators to optimize the base case")
       putStrLn ("pochoir -split-pointer $filename : " ++ breakline ++ 
               "Default Mode : split the interior and boundary region, and using C-style pointer to optimize the base case")

pProcess :: PMode -> Handle -> Handle -> IO ()
pProcess mode inh outh = 
    do ls <- hGetContents inh
       let pRevInitState = pInitState { pMode = mode }
       case runParser pParser pRevInitState "" $ stripWhite ls of
           Left err -> print err
           Right str -> hPutStrLn outh str


