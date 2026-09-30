//> Statements and State environment-class
package com.craftinginterpreters.lox;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class Environment {
//> enclosing-field
  final Environment enclosing;
//< enclosing-field
  private final List<Object> values = new ArrayList<>();
//> environment-constructors
  Environment() {
    enclosing = null;
  }

  Environment(Environment enclosing) {
    this.enclosing = enclosing;
  }
//< environment-constructors
  int define(Object value) {
    values.add(value);
    return values.size() - 1;
  }

//> Resolving and Binding get-at
  Object getAt(int distance, int slot) {
    return ancestor(distance).values.get(slot);
  }
//< Resolving and Binding get-at
//> Resolving and Binding assign-at
  void assignAt(int distance, int slot, Object value) {
    ancestor(distance).values.set(slot, value);
  }

  void assignLocal(int slot, Object value) {
    values.set(slot, value);
  }

  Environment ancestor(int distance) {
    Environment environment = this;

    for (int i = 0; i < distance; i++) {
      environment = environment.enclosing;
    }

    return environment;
  }
//< Resolving and Binding assign-at
//> omit
  @Override
  public String toString() {
    String result = values.toString();
    if (enclosing != null) {
      result += " -> " + enclosing.toString();
    }

    return result;
  }
//< omit
}
