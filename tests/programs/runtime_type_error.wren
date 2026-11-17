import "ifj25" for Ifj
class Program {
    static main() {
        var a
        a = 1
        Ifj.write("before\n")
        a = "x" + a
        Ifj.write("after\n")
    }
}
