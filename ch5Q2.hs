data Expression = Expression
  { evaluate :: () -> Double
  , display  :: () -> String
  }

number :: Double -> Expression
number value =
  Expression
    { evaluate = \() -> value
    , display  = \() -> show value
    }

addition :: Expression -> Expression -> Expression
addition left right =
  Expression
    { evaluate = \() ->
        evaluate left () + evaluate right ()
    , display = \() ->
        "(" ++ display left () ++
        " + " ++ display right () ++ ")"
    }

negation :: Expression -> Expression
negation expression =
  Expression
    { evaluate = \() -> -(evaluate expression ())
    , display  = \() -> "(-" ++ display expression () ++ ")"
    }

useExpression :: Expression -> IO ()
useExpression expression = do
  putStrLn ("Expression: " ++ display expression ())
  putStrLn ("Result: " ++ show (evaluate expression ()))

main :: IO ()
main = do
  let expression =
        addition
          (number 10)
          (negation (number 3))

  useExpression expression