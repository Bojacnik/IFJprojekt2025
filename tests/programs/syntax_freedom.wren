import "ifj25" for Ifj
class Program {
    static add(
        a,
        b) {
        return a +
            b
    }
    static main() {   // comment after brace
        var x /* block
        comment */
        x = add(1, 2)


        Ifj .
          write(x) // newline after the dot
        Ifj.write("\n")
        var y = 5
        Ifj.write(y)
        Ifj.write("\n")
        return
    }
}
