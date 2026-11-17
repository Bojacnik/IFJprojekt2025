import "ifj25" for Ifj
class Program {
    static main() {
        var a
        a = 0x1F + 1e2 + 2.5E-1
        Ifj.write(a)
        Ifj.write("\n")
        a = 7 / 2
        Ifj.write(a)
        Ifj.write(" ")
        a = Ifj.str(a)
        Ifj.write(a)
        Ifj.write("\n")
        a = 10 / 5
        Ifj.write(a)
        Ifj.write("\n")
        a = Ifj.floor(3.99)
        Ifj.write(a)
        Ifj.write("\n")
        a = 2 + 3 * 4 - 6 / 2
        Ifj.write(a)
        Ifj.write("\n")
        a = (2 + 3) * 4
        Ifj.write(a)
        Ifj.write("\n")
        if (1 == "1") {
            Ifj.write("bad")
        } else {
            Ifj.write("different types are not equal")
        }
        Ifj.write("\n")
        if (null == null) {
            Ifj.write("null == null")
        }
        Ifj.write("\n")
        a = "text"
        if (a is String) {
            Ifj.write("is String")
        }
        if (a is Num) {
            Ifj.write("bad")
        }
        Ifj.write("\n")
        if (3 >= 3) {
            if (2 <= 1) {
                Ifj.write("bad")
            } else {
                Ifj.write("relations ok")
            }
        }
        Ifj.write("\n")
        if (0) {
            Ifj.write("0 is truthy")
        }
        Ifj.write("\n")
    }
}
