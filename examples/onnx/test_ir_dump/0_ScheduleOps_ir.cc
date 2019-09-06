// attr [compute(T_multiply, 0x558bbce582c0)] realize_scope = ""
realize T_multiply([0, 1], [0, 8]) {
  produce T_multiply {
    for (ax1, 0, 8) {
      T_multiply(0, ax1) =(placeholder(0, ax1)*placeholder(0, ax1))
    }
  }
}
