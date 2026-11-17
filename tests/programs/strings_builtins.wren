import "ifj25" for Ifj
class Program {
    static main() {
        var s
        s = "Hello, world"
        var n
        n = Ifj.length(s)
        Ifj.write(n)
        Ifj.write("\n")
        var sub
        sub = Ifj.substring(s, 7, 12)
        Ifj.write(sub)
        Ifj.write("\n")
        sub = Ifj.substring(s, 5, 2)
        Ifj.write(sub)
        Ifj.write("\n")
        n = Ifj.ord(s, 0)
        Ifj.write(n)
        Ifj.write(" ")
        n = Ifj.ord(s, 99)
        Ifj.write(n)
        Ifj.write("\n")
        var c
        c = Ifj.chr(65)
        Ifj.write(c)
        Ifj.write("\n")
        n = Ifj.strcmp("abc", "abd")
        Ifj.write(n)
        n = Ifj.strcmp("b", "a")
        Ifj.write(n)
        n = Ifj.strcmp("x", "x")
        Ifj.write(n)
        Ifj.write("\n")
        var rep
        rep = "ab" * 3
        Ifj.write(rep)
        Ifj.write("\n")
        Ifj.write("tab:\there, quote:\" backslash:\\ hex:\x41\n")
        var ml
        ml = """
  multi "line"
  """
        Ifj.write(ml)
        Ifj.write("\n")
    }
}
