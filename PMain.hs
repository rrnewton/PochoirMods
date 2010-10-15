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
import System.Directory (doesFileExist)
import Data.Char (isSpace)
import qualified Data.Map as Map
--import Text.Regex.Posix
import Text.ParserCombinators.Parsec (runParser)

import PData
import PMainParser

main :: IO ()
main = do args <- getArgs
          let (inFile, obase) = parseArgs args
--          let (inFile:ppMode:_) = args
          fileExist <- doesFileExist inFile
          whilst (not fileExist) $ do
             putStrLn (inFile ++ " doesn't exist!")
             exitFailure
          let outFile = rename inFile
          putStrLn ("inFile = " ++ inFile ++ "; outFile = " ++ outFile)
          inh <- openFile inFile ReadMode
          outh <- openFile outFile WriteMode
          pProcess obase inh outh
          hClose inh
          hClose outh

whilst :: Bool -> IO () -> IO ()
whilst True action = action
whilst False action = return () 

rename :: String -> String
rename fname = name ++ "_pochoir" ++ suffix
            where (name, suffix) = break ('.' ==) fname
        
pInitState = ParserState { pObase = False, pState = Unrelated, pMacro = Map.empty, pArray = Map.empty, pStencil = Map.empty, pShape = Map.empty, pRange = Map.empty, pKernel = Map.empty}

parseArgs :: [String] -> (String, Bool)
parseArgs (a:[]) = (a, False)
parseArgs (a:b:_) 
    | a == "-split-shadow" = (b, False)
    | a == "-split-obase" = (b, True)
    | b == "-split-shadow" = (a, False)
    | b == "-split-obase" = (a, True)
    | otherwise = (a, False)

pProcess :: Bool -> Handle -> Handle -> IO ()
pProcess obase inh outh = 
    do ls <- hGetContents inh
       let pRevInitState = pInitState { pObase = obase }
       case runParser pParser pRevInitState "" $ stripWhite ls of
           Left err -> print err
           Right str -> hPutStrLn outh str


