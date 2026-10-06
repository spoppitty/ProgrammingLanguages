//> Classes lox-class
package com.craftinginterpreters.lox;

import java.util.List;
import java.util.Map;

/* Classes lox-class < Classes lox-class-callable
class LoxClass {
*/
//> lox-class-callable
class LoxClass extends LoxInstance implements LoxCallable {
  final String name;
  private final LoxClass superclass;
  private final Map<String, LoxFunction> methods;

  LoxClass(LoxClass metaclass,
           String name,
           LoxClass superclass,
           Map<String, LoxFunction> methods) {
    super(metaclass);
    this.name = name;
    this.superclass = superclass;
    this.methods = methods;
  }

//> find super
  LoxFunction findMethod(LoxInstance instance, String name) {
    LoxFunction method = null;
    LoxFunction inner = null;
    LoxClass klass = this;
    while (klass != null) {
      if (klass.methods.containsKey(name)) {
        inner = method;
        method = klass.methods.get(name);
      }
      klass = klass.superclass;
    }

    if (method != null) {
      return method.bind(instance, inner);
    }

    return null;
  }
//< find super
//> bind inner
  LoxFunction bind(LoxInstance instance, LoxFunction inner) {
    Environment environment = new Environment(closure);
    environment.define("this", instance);
    environment.define("inner", inner);
    return new LoxFunction(name, declaration, environment, isInitializer);
  }
//< bind inner
//> extension method
  void addMethod(String name, LoxFunction method) {
    methods.put(name, method);
  }
//< extension method

  @Override
  public String toString() {
    return name;
  }

//> lox-class-call-arity
  @Override
  public Object call(Interpreter interpreter, List<Object> arguments) {
      LoxInstance instance = new LoxInstance(this);

      LoxFunction initializer = findMethod(instance, "init");

      if (initializer != null) {
          initializer.call(interpreter, arguments);
      }

      return instance;
  }

  @Override
  public int arity() {
/* Classes lox-class-call-arity < Classes lox-initializer-arity
    return 0;
*/
//> lox-initializer-arity
    LoxFunction initializer = findMethod(null, "init");
    if (initializer == null) return 0;
    return initializer.arity();
//< lox-initializer-arity
  }
//< lox-class-call-arity
}
