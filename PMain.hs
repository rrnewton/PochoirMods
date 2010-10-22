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
import List
--import System.FilePath
import System.Directory (doesFileExist, removeFile)
import System.Cmd (rawSystem)
import Data.Char (isSpace)
import qualified Data.Map as Map
import Text.ParserCombinators.Parsec (runParser)

import PData
import PMainParser

main :: IO ()
main = do args <- getArgs
          whilst (null args == True) $ do
             printUsage
             exitFailure
          let (inFile, mode, debug, showFile) = parseArgs ("", PPointer, False, True) args
          fileExist <- doesFileExist inFile
          whilst (not fileExist) $ do
             putStrLn (inFile ++ " doesn't exist!")
             exitFailure
          whilst (mode == PError) $ do
             putStrLn ("command line argument error" ++ concat args)
             exitFailure
          let outFile = rename inFile
          putStrLn ("inFile = " ++ inFile ++ "; outFile = " ++ outFile)
          inh <- openFile inFile ReadMode
          outh <- openFile outFile WriteMode
          pProcess mode inh outh
          hClose inh
          hClose outh
--          let objInFile  = getObjFile inFile
--          let iccInFileArgs = if debug == False 
--                then objInFile ++ iccFlags ++ [inFile]
--                else objInFile ++ iccDebugFlags ++ [inFile]
--          putStrLn (icc ++ " " ++ concat (intersperse " " iccInFileArgs))
--          rawSystem icc iccInFileArgs
--          let objOutFile = getObjFile outFile
--          let iccOutFileArgs = if debug == False 
--                then objOutFile ++ iccFlags ++ [outFile]
--                else objOutFile ++ iccDebugFlags ++ [outFile]
--          putStrLn (icc ++ " " ++ concat (intersperse " " iccOutFileArgs))
--          rawSystem icc iccOutFileArgs
--          whilst (showFile == False) $ do
--             removeFile outFile

whilst :: Bool -> IO () -> IO ()
whilst True action = action
whilst False action = return () 

rename :: String -> String
rename fname = name ++ "_pochoir" ++ suffix
    where (name, suffix) = break ('.' ==) fname

getObjFile :: String -> [String]
getObjFile fname = ["-o"] ++ [name]
    where (name, suffix) = break ('.' ==) fname 

pInitState = ParserState { pMode = PPointer, pState = Unrelated, pMacro = Map.empty, pArray = Map.empty, pStencil = Map.empty, pShape = Map.empty, pRange = Map.empty, pKernel = Map.empty}

icc = "icc"

iccFlags = ["-O3", "-DNDEBUG", "-std=c++0x", "-Wall", "-Werror", "-ipo", "-I/opt/intel/composerxe-2011.0.048/compiler/include/cilk", "-I/home/yuantang/Git/Pochoir/ExecSpec_refine/"]

iccDebugFlags = ["-DDEBUG", "-O0", "-g3", "-std=c++0x", "-include", "cilk_stub.h", "-I/opt/intel/composerxe-2011.0.048/compiler/include/cilk", "-I/home/yuantang/Git/Pochoir/ExecSpec_refine/"]

parseArgs :: (String, PMode, Bool, Bool) -> [String] -> (String, PMode, Bool, Bool)
parseArgs (inFile, mode, debug, showFile) aL 
    | elem "-split-type-shadow" aL = 
        let l_mode = PTypeShadow
            aL' = delete "-split-type-shadow" aL
        in  parseArgs (inFile, l_mode, debug, showFile) aL'
    | elem "-split-pointer" aL =
        let l_mode = PPointer
            aL' = delete "-split-pointer" aL
        in  parseArgs (inFile, l_mode, debug, showFile) aL'
    | elem "-split-iter" aL =
        let l_mode = PIter
            aL' = delete "-split-iter" aL
        in  parseArgs (inFile, l_mode, debug, showFile) aL'
    | elem "-split-interior" aL =
        let l_mode = PInterior
            aL' = delete "-split-interior" aL
        in  parseArgs (inFile, l_mode, debug, showFile) aL'
    | elem "-split-macro-shadow" aL =
        let l_mode = PMacroShadow
            aL' = delete "-split-macro-shadow" aL
        in  parseArgs (inFile, l_mode, debug, showFile) aL'
    | elem "-showFile" aL =
        let l_showFile = True
            aL' = delete "-showFile" aL
        in  parseArgs (inFile, mode, debug, l_showFile) aL'
    | elem "-debug" aL =
        let l_debug = True
            aL' = delete "-debug" aL
        in  parseArgs (inFile, mode, l_debug, showFile) aL'
    | null aL == False =
        let l_file = head aL
        in  (l_file, mode, debug, showFile)
    | otherwise = 
        let l_mode = PError
        in  (inFile, mode, debug, showFile)

printUsage :: IO ()
printUsage = 
    do putStrLn ("Usage: ")
       putStrLn ("pp -split-type-shadow $filename : " ++ breakline ++ 
               "using type tricks to split the interior and boundary regions")
       putStrLn ("pp -split-macro-shadow $filename : " ++ breakline ++ 
               "using macro tricks to split the interior and boundary regions")
       putStrLn ("pp -split-iter $filename : " ++ breakline ++ 
               "split the interior and boundary region, and using iterators to optimize the base case")
       putStrLn ("pp -split-pointer $filename : " ++ breakline ++ 
               "Default : split the interior and boundary region, and using C-style pointer to optimize the base case")

pProcess :: PMode -> Handle -> Handle -> IO ()
pProcess mode inh outh = 
    do ls <- hGetContents inh
       let pRevInitState = pInitState { pMode = mode }
       case runParser pParser pRevInitState "" $ stripWhite ls of
           Left err -> print err
           Right str -> hPutStrLn outh str


