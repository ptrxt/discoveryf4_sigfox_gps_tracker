#!/usr/bin/env stack
{-# LANGUAGE StandaloneDeriving #-}
{-# LANGUAGE DeriveGeneric #-}
{-# LANGUAGE RecordWildCards #-}
{- Code Reflection from https://github.com/chpatrick/clang-pure/tree/master/examples
stack scripts/Reflect.hs main.cpp -Ilib/cocoOS_5.0.2/src
TODO: -- stack --resolver lts-12.8 script --package clang-pure,lens
-}

module Main where

import qualified Data.ByteString as BS
import           Data.Word
import           Language.C.Clang  --  stack install clang-pure
import           Language.C.Clang.Cursor
import           Control.Lens
import           System.Environment
import           Text.Pretty.Simple (pPrint)  -- stack install pretty-simple

-- Convert the cursor into a text array pattern for matching.
getPattern :: Cursor -> [BS.ByteString]
getPattern cursor =
  let kind = cursorKind cursor
  in case kind of
    DeclStmt {} -> [ cursorSpelling cursor ]  -- uint8_t task_id = task_create(...)
    CallExpr {} -> [ cursorSpelling cursor ] -- task_create(...)
    VarDecl {} -> [ cursorSpelling cursor ]

    IntegerLiteral {} -> [ cursorSpelling cursor ]
    FloatingLiteral {} -> [ cursorSpelling cursor ]
    CharacterLiteral {} -> [ cursorSpelling cursor ]
    _ -> []

-- Return the tokens for specific literals.
getTokens :: Cursor -> [BS.ByteString]
getTokens cursor =
  let kind = cursorKind cursor
      extent = cursorExtent cursor
  in case extent of
    Nothing -> []
    Just ext -> -- Only cursors with extents
      let tokenSet = tokenize ext
          tokens = tokenSetTokens tokenSet
          tokenList = take 10 (map tokenSpelling tokens) -- Limit to 10 tokens
      in case kind of
        IntegerLiteral {} -> tokenList
        FloatingLiteral {} -> tokenList
        CharacterLiteral {} -> tokenList
        UnaryOperator {} -> tokenList
        BinaryOperator {} -> tokenList
        
        CompoundStmt {} -> tokenList
        DeclStmt {} -> tokenList
        VarDecl {} -> tokenList

        FirstExpr {} -> tokenList
        CallExpr {} -> tokenList
        CXXBoolLiteralExpr {} -> tokenList
        _ -> []

-- Return the [ start, end ] location of the cursor.
getLocation :: Cursor -> [Location]
getLocation cursor =
  let extent = cursorExtent cursor
  in case extent of
    Nothing -> []
    Just ext -> 
      [ spellingLocation (rangeStart ext)
      , spellingLocation (rangeEnd ext)
      ]

-- Return the offset of this member in the parent struct.
getOffset :: Cursor -> [Word64]
getOffset cursor =
  let offset = offsetOfField cursor
  in case offset of
    Left _ -> []  -- TypeLayoutError
    Right os -> [os]

-- Recursively find all child cursors for specific lines and generate a text array pattern for matching.
getChildrenPattern :: Cursor -> [[BS.ByteString]]
getChildrenPattern root = 
  let patterns = root 
        ^.. cursorDescendantsF 
        . to (\c -> getPattern c)
      kind = cursorKind root
  in case kind of
    DeclStmt {} -> patterns -- uint8_t task_id = task_create(...)
    CallExpr {} -> patterns  -- task_create(...)
    _ -> []

-- Recursively find all child cursors and process them.
getChildren :: Cursor -> [
  ( CursorKind     -- Kind
  , BS.ByteString  -- Spelling
  , [[BS.ByteString]] -- Pattern
  , [BS.ByteString] -- Tokens
  , BS.ByteString  -- USR
  , [Word64]       -- Offset
  , [Location]     -- Location
  )]
getChildren root = root 
  ^.. cursorDescendantsF 
    . to (\c -> 
      ( cursorKind c
      , cursorSpelling c
      , getChildrenPattern c
      , getTokens c
      , cursorUSR c
      , getOffset c
      , getLocation c 
      ) )

main :: IO ()
main = do
  args <- getArgs
  case args of
    [] -> putStrLn "usage: find-classes file.cpp [clang opts]"

    paths -> do
      idx <- createIndex
      let path = head paths
      let options = tail paths
      tu <- parseTranslationUnit idx path options
      let root = translationUnitCursor tu
      let childList = getChildren root
      -- pPrint childList
      mapM_ print childList
