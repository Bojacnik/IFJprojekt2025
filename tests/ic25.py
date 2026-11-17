#!/usr/bin/env python3
"""Minimal IFJcode25 interpreter for local testing (NOT part of the submission).

Written from section 10 of the assignment. The official interpreter (ic25int)
is authoritative; where the spec is silent this one fails loudly instead of
guessing, so the generated code avoids those cases.

Usage: ic25.py program.code < input > output     (exit code as ic25int)
"""
import ctypes
import re
import sys

LIBC = ctypes.CDLL(None)


class Fail(Exception):
    def __init__(self, code, msg):
        super().__init__(msg)
        self.code = code


UNDEF = ("", None)  # defined but uninitialized variable


def c_hexfloat(x):
    buf = ctypes.create_string_buffer(64)
    LIBC.snprintf(buf, 64, b"%a", ctypes.c_double(x))
    return buf.value.decode()


def parse_string(raw):
    out, i = [], 0
    while i < len(raw):
        if raw[i] == "\\":
            if not re.fullmatch(r"\d{3}", raw[i + 1:i + 4]):
                raise Fail(51, f"bad escape in string@{raw}")
            out.append(chr(int(raw[i + 1:i + 4])))
            i += 4
        else:
            out.append(raw[i])
            i += 1
    return "".join(out)


def parse_const(tok):
    t, _, v = tok.partition("@")
    if t == "int":
        return ("int", int(v, 0))
    if t == "float":
        return ("float", float.fromhex(v) if "x" in v.lower() else float(v))
    if t == "string":
        return ("string", parse_string(v))
    if t == "bool":
        if v not in ("true", "false"):
            raise Fail(51, tok)
        return ("bool", v == "true")
    if t == "nil" and v == "nil":
        return ("nil", None)
    raise Fail(51, f"bad operand {tok}")


FLOAT_RE = re.compile(r"[+-]?(\d+\.?\d*([eE][+-]?\d+)?|\.\d+([eE][+-]?\d+)?|0[xX][0-9a-fA-F]+(\.[0-9a-fA-F]*)?([pP][+-]?\d+)?)")


class Machine:
    def __init__(self, lines):
        self.code, self.labels = [], {}
        for ln in lines:
            ln = ln.split("#", 1)[0].strip()
            if not ln:
                continue
            parts = ln.split()
            op = parts[0].upper()
            if op == "LABEL":
                if parts[1] in self.labels:
                    raise Fail(52, f"label redefined: {parts[1]}")
                self.labels[parts[1]] = len(self.code)
            self.code.append((op, parts[1:]))
        self.gf, self.frames, self.tf = {}, [], None
        self.stack, self.calls = [], []
        self.out = sys.stdout.buffer

    # --- frames and operands ---
    def frame(self, name):
        if name == "GF":
            return self.gf
        if name == "LF":
            if not self.frames:
                raise Fail(55, "no local frame")
            return self.frames[-1]
        if name == "TF":
            if self.tf is None:
                raise Fail(55, "no temporary frame")
            return self.tf
        raise Fail(51, f"bad frame {name}")

    def setvar(self, tok, value):
        fr, _, name = tok.partition("@")
        f = self.frame(fr)
        if name not in f:
            raise Fail(54, f"undefined variable {tok}")
        f[name] = value

    def get(self, tok, allow_undef=False):
        fr, _, name = tok.partition("@")
        if fr in ("GF", "LF", "TF"):
            f = self.frame(fr)
            if name not in f:
                raise Fail(54, f"undefined variable {tok}")
            v = f[name]
            if v is UNDEF and not allow_undef:
                raise Fail(56, f"uninitialized variable {tok}")
            return v
        return parse_const(tok)

    def label(self, name):
        if name not in self.labels:
            raise Fail(52, f"undefined label {name}")
        return self.labels[name]

    def pop(self):
        if not self.stack:
            raise Fail(56, "data stack empty")
        return self.stack.pop()

    # --- operations ---
    @staticmethod
    def arith(op, a, b):
        if a[0] != b[0] or a[0] not in ("int", "float") or (op == "DIV" and a[0] != "float") \
                or (op == "IDIV" and a[0] != "int"):
            raise Fail(53, f"{op} on {a[0]}, {b[0]}")
        x, y = a[1], b[1]
        if op in ("DIV", "IDIV") and y == 0:
            raise Fail(57, "division by zero")
        r = {"ADD": lambda: x + y, "SUB": lambda: x - y, "MUL": lambda: x * y,
             "DIV": lambda: x / y, "IDIV": lambda: x // y}[op]()
        return (a[0], r)

    @staticmethod
    def rel(op, a, b):
        if op == "EQ":
            if a[0] != b[0] and "nil" not in (a[0], b[0]):
                raise Fail(53, f"EQ on {a[0]}, {b[0]}")
            return ("bool", a == b)
        if a[0] != b[0] or a[0] == "nil":
            raise Fail(53, f"{op} on {a[0]}, {b[0]}")
        return ("bool", a[1] < b[1] if op == "LT" else a[1] > b[1])

    @staticmethod
    def logic(op, a, b=None):
        if a[0] != "bool" or (b is not None and b[0] != "bool"):
            raise Fail(53, f"{op} on non-bool")
        if op == "NOT":
            return ("bool", not a[1])
        return ("bool", (a[1] and b[1]) if op == "AND" else (a[1] or b[1]))

    @staticmethod
    def convert(op, a):
        need = {"INT2FLOAT": "int", "FLOAT2INT": "float", "INT2CHAR": "int", "FLOAT2STR": "float",
                "INT2STR": "int"}[op]
        if a[0] != need:
            raise Fail(53, f"{op} on {a[0]}")
        if op == "INT2FLOAT":
            return ("float", float(a[1]))
        if op == "FLOAT2INT":
            return ("int", int(a[1]))
        if op == "INT2CHAR":
            if not 0 <= a[1] <= 255:
                raise Fail(58, "INT2CHAR out of range")
            return ("string", chr(a[1]))
        if op == "FLOAT2STR":
            return ("string", "%.2f" % a[1])
        return ("string", str(a[1]))

    @staticmethod
    def str_index(op, s, i):
        if s[0] != "string" or i[0] != "int":
            raise Fail(53, f"{op} operand types")
        if not 0 <= i[1] < len(s[1]):
            raise Fail(58, f"{op} index out of range")
        return ("int", ord(s[1][i[1]])) if op == "STRI2INT" else ("string", s[1][i[1]])

    def read(self, typ):
        line = sys.stdin.readline()
        if line == "":
            return ("nil", None)
        line = line[:-1] if line.endswith("\n") else line
        if typ == "string":
            return ("string", line)
        if typ == "int":
            return ("int", int(line)) if re.fullmatch(r"[+-]?\d+", line) else ("nil", None)
        if typ == "float":
            if not FLOAT_RE.fullmatch(line):
                return ("nil", None)
            return ("float", float.fromhex(line) if "x" in line.lower() else float(line))
        if typ == "bool":
            return ("bool", line.lower() == "true")
        raise Fail(51, f"bad READ type {typ}")

    def write(self, v):
        t, x = v
        if t == "int":
            s = str(x)
        elif t == "float":
            s = c_hexfloat(x)
        elif t == "string":
            s = x
        elif t == "bool":
            s = "true" if x else "false"
        else:
            raise Fail(60, "WRITE nil: behaviour not specified, the compiler should not emit it")
        self.out.write(s.encode("latin-1"))

    def run(self):
        pc, steps = 0, 0
        while pc < len(self.code):
            op, a = self.code[pc]
            pc += 1
            steps += 1
            if steps > 50_000_000:
                raise Fail(60, "step limit")
            stacked = op.endswith("S") and op not in ("CLEARS", "PUSHS", "POPS", "ISINT", "TYPES") \
                and op[:-1] in ("ADD", "SUB", "MUL", "DIV", "IDIV", "LT", "GT", "EQ", "AND", "OR", "NOT",
                                "INT2FLOAT", "FLOAT2INT", "INT2CHAR", "STRI2INT", "FLOAT2STR", "INT2STR",
                                "JUMPIFEQ", "JUMPIFNEQ")
            if op in ("ISINTS", "TYPES"):
                stacked = True
            base = op[:-1] if stacked else op

            if base in ("ADD", "SUB", "MUL", "DIV", "IDIV", "LT", "GT", "EQ", "AND", "OR", "STRI2INT"):
                if stacked:
                    y, x = self.pop(), self.pop()
                else:
                    x, y = self.get(a[1]), self.get(a[2])
                if base in ("LT", "GT", "EQ"):
                    r = self.rel(base, x, y)
                elif base in ("AND", "OR"):
                    r = self.logic(base, x, y)
                elif base == "STRI2INT":
                    r = self.str_index(base, x, y)
                else:
                    r = self.arith(base, x, y)
                self.stack.append(r) if stacked else self.setvar(a[0], r)
            elif base in ("NOT", "INT2FLOAT", "FLOAT2INT", "INT2CHAR", "FLOAT2STR", "INT2STR", "ISINT", "TYPE"):
                x = self.pop() if stacked else self.get(a[1], allow_undef=(base == "TYPE"))
                if base == "NOT":
                    r = self.logic("NOT", x)
                elif base == "ISINT":
                    if x[0] not in ("int", "float"):
                        raise Fail(53, "ISINT on non-number")
                    r = ("bool", x[0] == "int" or float(x[1]).is_integer())
                elif base == "TYPE":
                    r = ("string", x[0])
                else:
                    r = self.convert(base, x)
                self.stack.append(r) if stacked else self.setvar(a[0], r)
            elif base in ("JUMPIFEQ", "JUMPIFNEQ"):
                if stacked:
                    y, x = self.pop(), self.pop()
                else:
                    x, y = self.get(a[1]), self.get(a[2])
                eq = self.rel("EQ", x, y)[1]
                target = self.label(a[0])
                if eq == (base == "JUMPIFEQ"):
                    pc = target
            elif op == "MOVE":
                self.setvar(a[0], self.get(a[1]))
            elif op == "CREATEFRAME":
                self.tf = {}
            elif op == "PUSHFRAME":
                self.frames.append(self.frame("TF"))
                self.tf = None
            elif op == "POPFRAME":
                self.tf = self.frame("LF")
                self.frames.pop()
            elif op == "DEFVAR":
                fr, _, name = a[0].partition("@")
                f = self.frame(fr)
                if name in f:
                    raise Fail(52, f"variable redefined: {a[0]}")
                f[name] = UNDEF
            elif op == "CALL":
                self.calls.append(pc)
                pc = self.label(a[0])
            elif op == "RETURN":
                if not self.calls:
                    raise Fail(56, "RETURN with empty call stack")
                pc = self.calls.pop()
            elif op == "PUSHS":
                self.stack.append(self.get(a[0]))
            elif op == "POPS":
                self.setvar(a[0], self.pop())
            elif op == "CLEARS":
                self.stack.clear()
            elif op == "READ":
                self.setvar(a[0], self.read(a[1]))
            elif op == "WRITE":
                self.write(self.get(a[0]))
            elif op == "CONCAT":
                x, y = self.get(a[1]), self.get(a[2])
                if x[0] != "string" or y[0] != "string":
                    raise Fail(53, "CONCAT on non-string")
                self.setvar(a[0], ("string", x[1] + y[1]))
            elif op == "STRLEN":
                x = self.get(a[1])
                if x[0] != "string":
                    raise Fail(53, "STRLEN on non-string")
                self.setvar(a[0], ("int", len(x[1])))
            elif op == "GETCHAR":
                self.setvar(a[0], self.str_index("GETCHAR", self.get(a[1]), self.get(a[2])))
            elif op == "LABEL":
                pass
            elif op == "JUMP":
                pc = self.label(a[0])
            elif op == "EXIT":
                v = self.get(a[0])
                if v[0] != "int" or not 0 <= v[1] <= 49:
                    raise Fail(57, "bad EXIT value")
                return v[1]
            elif op in ("BREAK", "DPRINT"):
                pass
            else:
                raise Fail(51, f"unknown instruction {op}")
        return 0


def main():
    with open(sys.argv[1], encoding="latin-1") as f:
        lines = f.read().split("\n")
    if not lines or lines[0].strip().upper() != ".IFJCODE25":
        print("missing .IFJcode25 header", file=sys.stderr)
        sys.exit(51)
    try:
        code = Machine(lines[1:]).run()
    except Fail as e:
        sys.stdout.flush()
        print(f"ic25: error {e.code}: {e}", file=sys.stderr)
        code = e.code
    sys.stdout.flush()
    sys.exit(code)


if __name__ == "__main__":
    main()
