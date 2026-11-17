import "ifj25" for Ifj
class Program {
    static main() {
        var s
        s = Ifj.read_str()
        var n
        n = Ifj.read_num()
        var bad
        bad = Ifj.read_num()
        var eof
        eof = Ifj.read_str()
        Ifj.write(s)
        Ifj.write("|")
        Ifj.write(n)
        Ifj.write("|")
        Ifj.write(bad)
        Ifj.write("|")
        Ifj.write(eof)
        Ifj.write("\n")
    }
}
