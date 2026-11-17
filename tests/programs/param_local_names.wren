import "ifj25" for Ifj
class Program {
    static f(a) {
        var p
        p = "local p"
        Ifj.write(a)
        Ifj.write(" ")
        Ifj.write(p)
        Ifj.write("\n")
    }
    static main() {
        f("param a")
    }
}
