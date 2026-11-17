import "ifj25" for Ifj
class Program {
    static main() {
        var i
        i = 0
        var s
        s = ""
        while (i < 3) {
            var i2
            i2 = i * 2
            var tmp
            tmp = Ifj.str(i2)
            s = s + tmp
            i = i + 1
        }
        Ifj.write(s)
        Ifj.write("\n")
        var x
        x = "outer"
        {
            var x
            x = "inner"
            Ifj.write(x)
        }
        Ifj.write(x)
        Ifj.write("\n")
    }
}
