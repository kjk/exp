/**
 * MSVC C++ name demangler.
 *
 * Decodes Microsoft Visual C++ decorated/mangled names (those starting with '?')
 * back into human-readable C++ declarations.
 */

class Demangler {
  private s: string;
  private pos: number;
  private nameBackrefs: string[] = [];
  private typeBackrefs: string[] = [];

  constructor(s: string) {
    this.s = s;
    this.pos = 0;
  }

  private peek(): string {
    return this.pos < this.s.length ? this.s[this.pos] : "";
  }

  private peekAt(offset: number): string {
    const idx = this.pos + offset;
    return idx < this.s.length ? this.s[idx] : "";
  }

  private consume(): string {
    return this.s[this.pos++] ?? "";
  }

  private consumeIf(ch: string): boolean {
    if (this.s[this.pos] === ch) {
      this.pos++;
      return true;
    }
    return false;
  }

  private atEnd(): boolean {
    return this.pos >= this.s.length;
  }

  demangle(): string {
    if (!this.s.startsWith("?")) return this.s;
    this.pos = 1;

    try {
      return this.parseSymbol();
    } catch {
      return this.s;
    }
  }

  private parseSymbol(): string {
    const { qualifiedName, specialPrefix, className } = this.parseQualifiedName();

    if (this.atEnd()) {
      return qualifiedName;
    }

    // Data members / static variables
    const ch = this.peek();
    if ("012345".includes(ch)) {
      return this.parseDataMember(qualifiedName);
    }

    // Function encoding
    return this.parseFunctionEncoding(qualifiedName, specialPrefix, className);
  }

  private parseQualifiedName(): { qualifiedName: string; specialPrefix: string; className: string } {
    let specialPrefix = "";
    let baseName: string;

    if (this.peek() === "?") {
      this.consume(); // second ?
      const result = this.parseSpecialName();
      baseName = result.name;
      specialPrefix = result.prefix;
    } else {
      baseName = this.parseUnqualifiedName();
    }

    // Parse scope (namespaces/classes) until @@
    const scopes: string[] = [];
    while (!this.atEnd() && this.peek() !== "@") {
      const scope = this.parseScopePart();
      if (scope === "") break;
      scopes.push(scope);
    }

    // Consume the @@ terminator
    this.consumeIf("@");
    this.consumeIf("@");

    // The first scope is the immediate containing class (for ctor/dtor)
    const className = scopes.length > 0 ? scopes[0] : "";

    const qualifiedName = scopes.length > 0
      ? [...scopes].reverse().join("::") + "::" + baseName
      : baseName;

    return { qualifiedName, specialPrefix, className };
  }

  private parseSpecialName(): { name: string; prefix: string } {
    const ch = this.consume();
    switch (ch) {
      case "0": return { name: "{ctor}", prefix: "ctor" };
      case "1": return { name: "{dtor}", prefix: "dtor" };
      case "2": return { name: "operator new", prefix: "" };
      case "3": return { name: "operator delete", prefix: "" };
      case "4": return { name: "operator=", prefix: "" };
      case "5": return { name: "operator>>", prefix: "" };
      case "6": return { name: "operator<<", prefix: "" };
      case "7": return { name: "operator!", prefix: "" };
      case "8": return { name: "operator==", prefix: "" };
      case "9": return { name: "operator!=", prefix: "" };
      case "A": return { name: "operator[]", prefix: "" };
      case "B": return { name: "operator " + this.parseType(), prefix: "conv" };
      case "C": return { name: "operator->", prefix: "" };
      case "D": return { name: "operator*", prefix: "" };
      case "E": return { name: "operator++", prefix: "" };
      case "F": return { name: "operator--", prefix: "" };
      case "G": return { name: "operator-", prefix: "" };
      case "H": return { name: "operator+", prefix: "" };
      case "I": return { name: "operator&", prefix: "" };
      case "J": return { name: "operator->*", prefix: "" };
      case "K": return { name: "operator/", prefix: "" };
      case "L": return { name: "operator%", prefix: "" };
      case "M": return { name: "operator<", prefix: "" };
      case "N": return { name: "operator<=", prefix: "" };
      case "O": return { name: "operator>", prefix: "" };
      case "P": return { name: "operator>=", prefix: "" };
      case "Q": return { name: "operator,", prefix: "" };
      case "R": return { name: "operator()", prefix: "" };
      case "S": return { name: "operator~", prefix: "" };
      case "T": return { name: "operator^", prefix: "" };
      case "U": return { name: "operator|", prefix: "" };
      case "V": return { name: "operator&&", prefix: "" };
      case "W": return { name: "operator||", prefix: "" };
      case "X": return { name: "operator*=", prefix: "" };
      case "Y": return { name: "operator+=", prefix: "" };
      case "Z": return { name: "operator-=", prefix: "" };
      case "$": {
        // Template function: ??$name@templateargs@
        // The name follows until @, then template args until @
        const templateResult = this.parseTemplateNameAndArgs();
        return { name: templateResult, prefix: "" };
      }
      case "_": {
        const next = this.consume();
        switch (next) {
          case "0": return { name: "operator/=", prefix: "" };
          case "1": return { name: "operator%=", prefix: "" };
          case "2": return { name: "operator>>=", prefix: "" };
          case "3": return { name: "operator<<=", prefix: "" };
          case "4": return { name: "operator&=", prefix: "" };
          case "5": return { name: "operator|=", prefix: "" };
          case "6": return { name: "operator^=", prefix: "" };
          case "7": return { name: "`vftable'", prefix: "vftable" };
          case "8": return { name: "`vbtable'", prefix: "vbtable" };
          case "9": return { name: "`vcall'", prefix: "" };
          case "A": return { name: "`typeof'", prefix: "" };
          case "B": return { name: "`local static guard'", prefix: "guard" };
          case "D": return { name: "`vbase destructor'", prefix: "dtor" };
          case "E": return { name: "`vector deleting destructor'", prefix: "dtor" };
          case "F": return { name: "`default constructor closure'", prefix: "ctor" };
          case "G": return { name: "`scalar deleting destructor'", prefix: "dtor" };
          case "H": return { name: "`vector constructor iterator'", prefix: "" };
          case "I": return { name: "`vector destructor iterator'", prefix: "" };
          case "J": return { name: "`vector vbase constructor iterator'", prefix: "" };
          case "K": return { name: "`virtual displacement map'", prefix: "" };
          case "L": return { name: "`eh vector constructor iterator'", prefix: "" };
          case "M": return { name: "`eh vector destructor iterator'", prefix: "" };
          case "N": return { name: "`eh vector vbase constructor iterator'", prefix: "" };
          case "O": return { name: "`copy constructor closure'", prefix: "ctor" };
          case "R": {
            const r = this.consume();
            switch (r) {
              case "0": return { name: "`RTTI Type Descriptor'", prefix: "rtti" };
              case "1": {
                // RTTI Base Class Descriptor with encoded numbers
                // Skip the numbers
                this.parseEncodedNumber();
                this.parseEncodedNumber();
                this.parseEncodedNumber();
                this.parseEncodedNumber();
                return { name: "`RTTI Base Class Descriptor'", prefix: "rtti" };
              }
              case "2": return { name: "`RTTI Base Class Array'", prefix: "rtti" };
              case "3": return { name: "`RTTI Class Hierarchy Descriptor'", prefix: "rtti" };
              case "4": return { name: "`RTTI Complete Object Locator'", prefix: "rtti" };
              default: return { name: "`RTTI'", prefix: "rtti" };
            }
          }
          case "S": return { name: "`local vftable'", prefix: "vftable" };
          case "T": return { name: "`local vftable constructor closure'", prefix: "" };
          case "U": return { name: "operator new[]", prefix: "" };
          case "V": return { name: "operator delete[]", prefix: "" };
          default: return { name: `??_${next}`, prefix: "" };
        }
      }
      default:
        return { name: `??${ch}`, prefix: "" };
    }
  }

  private parseTemplateNameAndArgs(addBackref: boolean = true): string {
    let templateName: string;

    // Check if template name is a special/operator name (starts with ?)
    if (this.peek() === "?") {
      this.consume();
      const special = this.parseSpecialName();
      templateName = special.name;
      // No @ to consume after special names - they're fixed-length codes
    } else {
      // Regular identifier terminated by @
      templateName = "";
      while (!this.atEnd() && this.peek() !== "@") {
        templateName += this.consume();
      }
      if (this.peek() === "@") this.consume();
    }

    // Add the bare template name as a back-reference BEFORE parsing args
    // This is needed so that template args can reference the template name by index
    if (addBackref && templateName && this.nameBackrefs.length < 10) {
      this.nameBackrefs.push(templateName);
    }

    // Parse template arguments until @
    const args = this.parseTemplateArgList();

    // Build template result with proper spacing for nested templates
    const argsStr = args.join(",");
    const closing = argsStr.endsWith(">") ? " >" : ">";
    const result = templateName + "<" + argsStr + closing;

    // Also add the full template form after args for later references
    if (addBackref && result && this.nameBackrefs.length < 10) {
      this.nameBackrefs.push(result);
    }
    return result;
  }

  private parseTemplateArgList(): string[] {
    const args: string[] = [];
    while (!this.atEnd() && this.peek() !== "@") {
      if (this.peek() === "$") {
        this.consume();
        const arg = this.parseTemplateArg();
        if (arg !== "") args.push(arg);
      } else {
        const argType = this.parseType();
        if (argType !== "") args.push(argType);
      }
    }
    if (this.peek() === "@") this.consume();
    return args;
  }

  private parseTemplateArg(): string {
    const ch = this.peek();
    if (ch === "$") {
      this.consume();
      const next = this.consume();
      if (next === "V") return ""; // empty parameter pack
      if (next === "S") return ""; // empty
      if (next === "0") return this.parseEncodedNumber().toString();
      if (next === "1") return (-this.parseEncodedNumber()).toString();
      if (next === "T") return "std::nullptr_t";
      if (next === "Q") {
        // $$Q = rvalue reference in template context, parse as type
        return this.parseType() + "&&";
      }
      if (next === "C") {
        // type with const/volatile qualifiers
        const cv = this.parseCVQualifier();
        const baseType = this.parseType();
        return cv ? baseType + " " + cv : baseType;
      }
      if (next === "B") {
        // array in template
        return this.parseType();
      }
      return "";
    }
    // Check for integer constant (0-9 or ?)
    if (ch === "0") {
      this.consume();
      return this.parseEncodedNumber().toString();
    }
    if (ch === "1") {
      this.consume();
      return (-this.parseEncodedNumber()).toString();
    }
    if (ch === "2") {
      this.consume();
      return this.parseEncodedNumber().toString();
    }
    // Unknown $ sequence - try parsing as type from current position
    return this.parseType();
  }

  private parseEncodedNumber(): number {
    const negative = this.consumeIf("?");

    const ch = this.peek();
    if (ch >= "0" && ch <= "9") {
      // Single digit encoding: value = digit + 1
      this.consume();
      const val = parseInt(ch) + 1;
      return negative ? -val : val;
    }

    // Multi-digit: A=0, B=1, ..., P=15, terminated by @
    let val = 0;
    while (!this.atEnd() && this.peek() !== "@") {
      const c = this.consume();
      const digit = c.charCodeAt(0) - "A".charCodeAt(0);
      val = val * 16 + digit;
    }
    if (this.peek() === "@") this.consume();
    return negative ? -val : val;
  }

  private parseUnqualifiedName(addBackref: boolean = true): string {
    // Check for back-reference digit
    const ch = this.peek();
    if (ch >= "0" && ch <= "9") {
      this.consume();
      return this.nameBackrefs[parseInt(ch)] ?? `{ref${ch}}`;
    }

    // Check for template: ?$
    if (ch === "?" && this.peekAt(1) === "$") {
      this.consume(); // ?
      this.consume(); // $
      return this.parseTemplateNameAndArgs(addBackref);
    }

    // Regular identifier terminated by @
    let name = "";
    while (!this.atEnd() && this.peek() !== "@") {
      name += this.consume();
    }
    if (this.peek() === "@") this.consume();

    if (addBackref && name && this.nameBackrefs.length < 10) {
      this.nameBackrefs.push(name);
    }
    return name;
  }

  private parseScopePart(addBackref: boolean = true): string {
    const ch = this.peek();

    // Back-reference digit
    if (ch >= "0" && ch <= "9") {
      this.consume();
      return this.nameBackrefs[parseInt(ch)] ?? `{scope${ch}}`;
    }

    // Template in scope: ?$
    if (ch === "?" && this.peekAt(1) === "$") {
      this.consume(); // ?
      this.consume(); // $
      return this.parseTemplateNameAndArgs(addBackref);
    }

    // Anonymous namespace: ?A
    if (ch === "?" && this.peekAt(1) === "A") {
      this.consume(); // ?
      this.consume(); // A
      // Skip the hex id
      while (!this.atEnd() && this.peek() !== "@") {
        this.consume();
      }
      if (this.peek() === "@") this.consume();
      const name = "`anonymous namespace'";
      if (addBackref && this.nameBackrefs.length < 10) {
        this.nameBackrefs.push(name);
      }
      return name;
    }

    // Other special ? scopes
    if (ch === "?") {
      this.consume();
      const next = this.peek();

      // Numbered namespace/scope
      if (next >= "0" && next <= "9") {
        const num = this.parseEncodedNumber();
        return `\`${num}'`;
      }

      // Nested special name used as scope (like ??0CtorName@@...)
      if (next === "?") {
        this.consume();
        const special = this.parseSpecialName();
        const innerScopes: string[] = [];
        while (!this.atEnd() && this.peek() !== "@") {
          const scope = this.parseScopePart(addBackref);
          if (scope === "") break;
          innerScopes.push(scope);
        }
        if (this.peek() === "@") this.consume();
        const qualName = innerScopes.length > 0
          ? [...innerScopes].reverse().join("::") + "::" + special.name
          : special.name;
        if (addBackref && this.nameBackrefs.length < 10) {
          this.nameBackrefs.push(qualName);
        }
        return qualName;
      }

      // Lambda or unknown special scope
      let unknown = "";
      while (!this.atEnd() && this.peek() !== "@") {
        unknown += this.consume();
      }
      if (this.peek() === "@") this.consume();
      if (addBackref && unknown && this.nameBackrefs.length < 10) {
        this.nameBackrefs.push(unknown);
      }
      return unknown;
    }

    // Regular name
    let name = "";
    while (!this.atEnd() && this.peek() !== "@") {
      name += this.consume();
    }
    if (this.peek() === "@") this.consume();

    if (addBackref && name && this.nameBackrefs.length < 10) {
      this.nameBackrefs.push(name);
    }
    return name;
  }

  private parseFunctionEncoding(qualifiedName: string, specialPrefix: string, className: string): string {
    const accessChar = this.consume();
    let isStatic = false;
    let isVirtual = false;
    let isMember = false;

    switch (accessChar) {
      // Private
      case "A": case "B": isMember = true; break;
      case "C": case "D": isMember = true; isStatic = true; break;
      case "E": case "F": isMember = true; isVirtual = true; break;
      // Protected
      case "I": case "J": isMember = true; break;
      case "K": case "L": isMember = true; isStatic = true; break;
      case "M": case "N": isMember = true; isVirtual = true; break;
      // Public
      case "Q": case "R": isMember = true; break;
      case "S": case "T": isMember = true; isStatic = true; break;
      case "U": case "V": isMember = true; isVirtual = true; break;
      case "W": case "X": isMember = true; isVirtual = true; break;
      // Non-member
      case "Y": case "Z": break;
      default:
        return qualifiedName;
    }

    // Parse 'this' qualifiers for non-static member functions
    if (isMember && !isStatic) {
      // 64-bit 'E' prefix
      this.consumeIf("E");
      // cv-qualifier on 'this'
      const cv = this.peek();
      if ("ABCD".includes(cv)) this.consume();
    }

    // Calling convention
    const callingConv = this.parseCallingConvention();

    // Return type
    let returnType = "";
    if (specialPrefix === "ctor" || specialPrefix === "dtor") {
      // Constructors/destructors have @ as return type placeholder
      this.consumeIf("@");
    } else if (specialPrefix === "conv") {
      // Conversion operators - return type is encoded in the name
      returnType = "";
    } else {
      returnType = this.parseType();
    }

    // Parameters
    const params = this.parseParamList();

    // Build the display name
    let displayName = qualifiedName;
    if (specialPrefix === "ctor" || specialPrefix === "dtor") {
      // Strip template args from class name for ctor/dtor display
      let shortClassName = className;
      const angleBracket = className.indexOf("<");
      if (angleBracket > 0) {
        shortClassName = className.substring(0, angleBracket);
      }
      // Find the base name placeholder in qualified name and replace
      const lastSep = qualifiedName.lastIndexOf("::");
      if (lastSep >= 0) {
        const prefix = qualifiedName.substring(0, lastSep + 2);
        displayName = prefix + (specialPrefix === "dtor" ? "~" : "") + shortClassName;
      } else {
        displayName = (specialPrefix === "dtor" ? "~" : "") + shortClassName;
      }
    }

    // Build result
    let result = "";
    if (isVirtual) result += "virtual ";
    if (isStatic) result += "static ";
    const retStr = returnType || (specialPrefix === "ctor" || specialPrefix === "dtor" ? "void" : "");
    if (retStr) result += retStr + " ";
    result += callingConv + " ";
    result += displayName;
    result += "(" + params + ")";

    return result;
  }

  private parseCallingConvention(): string {
    const ch = this.consume();
    switch (ch) {
      case "A": case "B": return "__cdecl";
      case "C": case "D": return "__pascal";
      case "E": case "F": return "__thiscall";
      case "G": case "H": return "__stdcall";
      case "I": case "J": return "__fastcall";
      case "K": case "L": return "";
      case "M": case "N": return "__clrcall";
      case "O": case "P": return "__eabi";
      case "Q": return "__vectorcall";
      default:
        this.pos--;
        return "__cdecl";
    }
  }

  private parseParamList(): string {
    // X = void (no params)
    if (this.peek() === "X") {
      this.consume();
      this.consumeIf("Z");
      return "";
    }

    const params: string[] = [];
    while (!this.atEnd()) {
      const ch = this.peek();

      // End of param list
      if (ch === "@") {
        this.consume();
        this.consumeIf("Z");
        break;
      }
      if (ch === "Z") {
        this.consume();
        params.push("...");
        break;
      }

      // Type back-reference (digit in param context)
      if (ch >= "0" && ch <= "9") {
        this.consume();
        const ref = this.typeBackrefs[parseInt(ch)];
        if (ref) params.push(ref);
        continue;
      }

      // Parse a new type and add it as a back-reference
      const savedBackrefCount = this.typeBackrefs.length;
      const paramType = this.parseType();
      if (paramType && this.typeBackrefs.length === savedBackrefCount && this.typeBackrefs.length < 10) {
        this.typeBackrefs.push(paramType);
      }
      params.push(paramType);
    }

    return params.join(", ");
  }

  private parseType(): string {
    const ch = this.peek();

    switch (ch) {
      case "P": case "Q": case "R": case "S":
        return this.parsePointerType();
      case "A":
        return this.parseReferenceType("&");
      case "$":
        return this.parseDollarType();
      default:
        return this.parseBasicOrClassType();
    }
  }

  private parseDollarType(): string {
    this.consume(); // $
    const next = this.peek();
    if (next === "$") {
      this.consume(); // second $
      const code = this.consume();
      switch (code) {
        case "Q": // rvalue reference
          return this.parseModifiedRefType("&&");
        case "R": // volatile rvalue reference
          return this.parseModifiedRefType("&&");
        case "A": // managed reference
          return this.parseReferenceType("&");
        case "T":
          return "std::nullptr_t";
        case "C": {
          // type with const/volatile qualifiers
          const cv = this.parseCVQualifier();
          const baseType = this.parseType();
          return cv ? baseType + " " + cv : baseType;
        }
        case "V": // empty
          return "";
        case "B": {
          // Array type or type with size
          this.parseEncodedNumber(); // array dimension
          return this.parseType();
        }
        default:
          return this.parseType();
      }
    }
    // Single $ followed by something - might be in template context
    // Back up and let caller handle
    this.pos--;
    return this.parseBasicOrClassType();
  }

  private parsePointerType(): string {
    const ptrKind = this.consume(); // P, Q, R, or S

    // 64-bit pointer
    this.consumeIf("E");

    // CV qualifier on the pointee
    const cv = this.parseCVQualifier();

    // Check for function pointer (6) or member function pointer (8)
    if (this.peek() === "6") {
      this.consume();
      return this.parseFunctionPtrType(cv, "*");
    }
    if (this.peek() === "8") {
      this.consume();
      return this.parseMemberFunctionPtrType(cv);
    }

    const baseType = this.parseType();
    let result = cv ? cv + " " + baseType : baseType;
    result += "*";

    // For Q (const ptr), R (volatile ptr), S (const volatile ptr)
    if (ptrKind === "Q") result += " const";
    else if (ptrKind === "R") result += " volatile";
    else if (ptrKind === "S") result += " const volatile";

    return result;
  }

  private parseReferenceType(refSymbol: string): string {
    this.consume(); // A

    // 64-bit
    this.consumeIf("E");

    const cv = this.parseCVQualifier();

    // Function reference
    if (this.peek() === "6") {
      this.consume();
      return this.parseFunctionPtrType(cv, refSymbol);
    }

    const baseType = this.parseType();
    let result = cv ? cv + " " + baseType : baseType;
    result += refSymbol;
    return result;
  }

  private parseModifiedRefType(refSymbol: string): string {
    // 64-bit
    this.consumeIf("E");

    const cv = this.parseCVQualifier();

    if (this.peek() === "6") {
      this.consume();
      return this.parseFunctionPtrType(cv, refSymbol);
    }

    const baseType = this.parseType();
    let result = cv ? cv + " " + baseType : baseType;
    result += refSymbol;
    return result;
  }

  private parseFunctionPtrType(cv: string, ptrSymbol: string): string {
    const callingConv = this.parseCallingConvention();
    const returnType = this.parseType();
    const params = this.parseParamList();

    let inner = callingConv;
    if (ptrSymbol) inner += " " + ptrSymbol;
    let result = `${returnType} (${inner.trim()})(${params})`;
    if (cv) result += " " + cv;
    return result;
  }

  private parseMemberFunctionPtrType(cv: string): string {
    // Parse the class scope
    const className = this.parseQualifiedTypeName();

    const callingConv = this.parseCallingConvention();

    // member function qualifiers
    this.consumeIf("E");
    const memberCv = this.parseCVQualifier();

    const returnType = this.parseType();
    const params = this.parseParamList();

    let inner = `${callingConv} ${className}::*`;
    let result = `${returnType} (${inner.trim()})(${params})`;
    if (memberCv) result += " " + memberCv;
    if (cv) result += " " + cv;
    return result;
  }

  private parseCVQualifier(): string {
    const ch = this.peek();
    switch (ch) {
      case "A": this.consume(); return "";
      case "B": this.consume(); return "const";
      case "C": this.consume(); return "volatile";
      case "D": this.consume(); return "const volatile";
      default: return "";
    }
  }

  private parseBasicOrClassType(): string {
    const ch = this.consume();
    switch (ch) {
      case "C": return "signed char";
      case "D": return "char";
      case "E": return "unsigned char";
      case "F": return "short";
      case "G": return "unsigned short";
      case "H": return "int";
      case "I": return "unsigned int";
      case "J": return "long";
      case "K": return "unsigned long";
      case "M": return "float";
      case "N": return "double";
      case "O": return "long double";
      case "X": return "void";
      case "_": {
        const next = this.consume();
        switch (next) {
          case "D": return "__int8";
          case "E": return "unsigned __int8";
          case "F": return "__int16";
          case "G": return "unsigned __int16";
          case "H": return "__int32";
          case "I": return "unsigned __int32";
          case "J": return "__int64";
          case "K": return "unsigned __int64";
          case "L": return "__int128";
          case "M": return "unsigned __int128";
          case "N": return "bool";
          case "W": return "wchar_t";
          case "S": return "char16_t";
          case "U": return "char32_t";
          default: return `_${next}`;
        }
      }
      case "T": case "U": case "V": {
        // T=union, U=struct, V=class
        const name = this.parseQualifiedTypeName();
        return name;
      }
      case "W": {
        // Enum: W followed by underlying type code, then name
        this.consume(); // underlying type (4=int, etc.)
        const name = this.parseQualifiedTypeName();
        return name;
      }
      case "Y": {
        // Array type: Y + dimension count + sizes... + element type
        const dims = this.parseEncodedNumber();
        const sizes: number[] = [];
        for (let i = 0; i < dims; i++) {
          sizes.push(this.parseEncodedNumber());
        }
        const elemType = this.parseType();
        return elemType + sizes.map(s => `[${s}]`).join("");
      }
      case "?": {
        // Modified type
        const cv = this.parseCVQualifier();
        const baseType = this.parseType();
        return cv ? cv + " " + baseType : baseType;
      }
      default:
        return `{?${ch}}`;
    }
  }

  private parseQualifiedTypeName(): string {
    const parts: string[] = [];

    // First part: the type name itself
    const typeName = this.parseTypeNamePart();
    parts.push(typeName);

    // Remaining scope parts until @
    while (!this.atEnd() && this.peek() !== "@") {
      const scope = this.parseScopePart();
      if (scope === "") break;
      parts.push(scope);
    }
    if (this.peek() === "@") this.consume(); // terminating @

    if (parts.length === 1) return parts[0];
    return [...parts.slice(1)].reverse().join("::") + "::" + parts[0];
  }

  private parseTypeNamePart(): string {
    const ch = this.peek();

    // Back-reference
    if (ch >= "0" && ch <= "9") {
      this.consume();
      return this.nameBackrefs[parseInt(ch)] ?? `{ref${ch}}`;
    }

    // Template: ?$
    if (ch === "?" && this.peekAt(1) === "$") {
      this.consume(); // ?
      this.consume(); // $
      return this.parseTemplateNameAndArgs();
    }

    // Anonymous namespace: ?A
    if (ch === "?" && this.peekAt(1) === "A") {
      this.consume(); // ?
      this.consume(); // A
      while (!this.atEnd() && this.peek() !== "@") {
        this.consume();
      }
      if (this.peek() === "@") this.consume();
      const name = "`anonymous namespace'";
      if (this.nameBackrefs.length < 10) {
        this.nameBackrefs.push(name);
      }
      return name;
    }

    // Other ? prefix
    if (ch === "?") {
      this.consume();
      let inner = "";
      while (!this.atEnd() && this.peek() !== "@") {
        inner += this.consume();
      }
      if (this.peek() === "@") this.consume();
      return `?${inner}`;
    }

    // Regular identifier
    let name = "";
    while (!this.atEnd() && this.peek() !== "@") {
      name += this.consume();
    }
    if (this.peek() === "@") this.consume();

    if (name && this.nameBackrefs.length < 10) {
      this.nameBackrefs.push(name);
    }
    return name;
  }

  private parseDataMember(qualifiedName: string): string {
    this.consume(); // access code
    const type = this.parseType();
    return type + " " + qualifiedName;
  }
}

export function demangleCpp(s: string): string {
  if (!s.startsWith("?")) return s;
  const d = new Demangler(s);
  return d.demangle();
}
