var a : Int = 5

if a < 7 {
    a = 3
}

var b : Int? = 6

if b > 7 {
    b = 0
} else {
    b = 4
}

if let b {
    a = a + b
}
