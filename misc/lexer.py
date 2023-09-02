import sys

DEBUG = False
file1 = open("texto.txt")

Lines = file1.readlines()

lexer_list = {}
cnt: int

def find_namespace(txt: str) -> dict:

  txt_idx = txt.find("__")
  i = 0
  #if (txt.find("*") > -1):
  #  i = 1

  if (txt_idx > -1):
    return {"ns": txt[i:txt_idx], "next": txt[txt_idx + len("__"):]}

  return None

def find_object(txt: str) -> dict:
  txt_idx = txt.find(" ")
  if (txt_idx > -1):
    return {"typeObject": txt[0:txt_idx], "next": txt[txt_idx + len(" "):]}

  return None

def find_nullable(txt: str) -> dict:
  txt_idx = txt.find("**")
  if (txt_idx > -1):
    return {"nullable": True, "next": txt[txt_idx + len("**"):]}

  txt_idx = txt.find("*")
  if (txt_idx > -1):
    return {"nullable": True, "next": txt[txt_idx + len("*"):]}

  return {"nullable": False, "next": txt}

def find_name_object(txt: str) -> dict:
  txt_idx = txt.find(";")
  if (txt_idx > -1):
    return {"nameObject": txt[0:txt_idx], "next": txt[txt_idx+len(";"):]}

  return {"next": txt}

def find_array(txt: str) -> bool:
  if (txt != None):
    return (txt.find("int __size") > -1) or (txt.find("char **") > -1)

  return False

def find_attribute(txt: str) -> bool:
  if (txt != None):
    return (txt.find("/**") > -1) and (txt.find("attribute") > -1)

  return False

def unknown_lex(txt: str) -> bool:
  return not ((txt.find("/**") > -1) or (txt.find("};") > -1) or (txt.find("int __size") > -1))

def is_default_attribute(l: list, d: dict) -> bool:
  if (len(l) != 3):
    return False

  ret = True
  for n in l:
    tmp = d[n]["nameObject"]

    ret = ret and ((tmp == "uuid") or (tmp == "schemaVersion") or (tmp == "objectVersion"))
    if (ret == False):
      return False

  return True

def process_line(txt: str, prev_txt: str = None) -> dict:
  global lexer_list
  global cnt

  txt_tmp = None

  txt_idx = txt.find("//")
  if (txt_idx > -1):
    return

  txt_idx = txt.find("struct ")
  if (txt_idx > -1):
    txt_tmp = txt[txt_idx + len("struct "):]
    m_tmp = {"type": "object"}
    m_tmp["isArray"] = find_array(prev_txt)
    #maybe bug here
    m_tmp["isAttribute"] = find_attribute(prev_txt)
    tmp = find_namespace(txt_tmp)
    if (tmp != None):
      m_tmp["ns"] = tmp["ns"]
      txt_tmp = tmp["next"]
    tmp = find_object(txt_tmp)
    m_tmp["typeObject"] = tmp["typeObject"]
    txt_tmp = tmp["next"]
    tmp = find_nullable(txt_tmp)
    m_tmp["nullable"] = tmp["nullable"]
    txt_tmp = tmp["next"]
    tmp = find_namespace(txt_tmp)
    if (tmp != None):
      m_tmp["name_ns"] = tmp["ns"]
      txt_tmp = tmp["next"]
    tmp = find_name_object(txt_tmp)
    if ("nameObject" in tmp):
      m_tmp["nameObject"] = tmp["nameObject"]
    txt_tmp = tmp["next"]

    lexer_list[cnt] = m_tmp
    cnt = cnt + 1
    return

  txt_idx = txt.find("char *")
  if (txt_idx > -1):
    m_tmp = {"type": "string"}
    m_tmp["isArray"] = find_array(txt)
    txt_tmp = txt[txt_idx + len("char *"):]
    m_tmp["isAttribute"] = find_attribute(prev_txt)
    tmp = find_namespace(txt_tmp)
    if (tmp != None):
      m_tmp["name_ns"] = tmp["ns"]
      txt_tmp = tmp["next"]
    tmp = find_name_object(txt_tmp)
    if ("nameObject" in tmp):
      m_tmp["nameObject"] = tmp["nameObject"]
    txt_tmp = tmp["next"]

    lexer_list[cnt] = m_tmp
    cnt = cnt + 1
    return

  txt_idx = txt.find("enum xsd__boolean ")
  if (txt_idx > -1):
    txt_tmp = txt[txt_idx + len("enum xsd__boolean "):]
    m_tmp = {"type": "boolean"}
    if (find_array(prev_txt)):
      raise BaseException("Implement enum boolean " + prev_txt)

    if (find_array(prev_txt)):
      raise BaseException("Implement boolean array " + prev_txt)

    if (find_attribute(prev_txt)==True):
      raise BaseException("Implement support for boolean attribute")

    m_tmp["isAttribute"] = False
    tmp = find_nullable(txt_tmp)
    m_tmp["nullable"] = tmp["nullable"]
    txt_tmp = tmp["next"]
    tmp = find_namespace(txt_tmp)
    if (tmp != None):
      m_tmp["name_ns"] = tmp["ns"]
      txt_tmp = tmp["next"]
    tmp = find_name_object(txt_tmp)
    if ("nameObject" in tmp):
      m_tmp["nameObject"] = tmp["nameObject"]
    txt_tmp = tmp["next"]

    lexer_list[cnt] = m_tmp
    cnt = cnt + 1
    return

  txt_idx = txt.find("enum ")
  if (txt_idx > -1):
    txt_tmp = txt[txt_idx + len("enum "):]
    m_tmp = {"type": "enum"}
    #if (find_array(prev_txt)):
    #  raise BaseException("Implement enum array " + prev_txt)
    m_tmp["isArray"] = find_array(prev_txt)
    m_tmp["isAttribute"] = find_attribute(prev_txt)
    tmp = find_namespace(txt_tmp)
    m_tmp["ns"] = tmp["ns"]
    txt_tmp = tmp["next"]
    tmp = find_object(txt_tmp)
    m_tmp["typeObject"] = tmp["typeObject"]
    txt_tmp = tmp["next"]
    tmp = find_nullable(txt_tmp)
    m_tmp["nullable"] = tmp["nullable"]
    txt_tmp = tmp["next"]
    tmp = find_namespace(txt_tmp)
    if (tmp != None):
      m_tmp["name_ns"] = tmp["ns"]
      txt_tmp = tmp["next"]
    tmp = find_name_object(txt_tmp)
    if ("nameObject" in tmp):
      m_tmp["nameObject"] = tmp["nameObject"]
    txt_tmp = tmp["next"]

    lexer_list[cnt] = m_tmp
    cnt = cnt + 1
    return

  txt_idx = txt.find("time_t ")
  if (txt_idx > -1):
    txt_tmp = txt[txt_idx + len("time_t "):]
    m_tmp = {"type": "time"}

    if (find_array(prev_txt)):
      raise BaseException("Implement time_t array " + prev_txt)

    if (find_attribute(prev_txt)==True):
      raise BaseException("Implement support for time_t attribute")

    m_tmp["isAttribute"] = False

    tmp = find_nullable(txt_tmp)
    m_tmp["nullable"] = tmp["nullable"]
    txt_tmp = tmp["next"]
    tmp = find_namespace(txt_tmp)
    if (tmp != None):
      m_tmp["name_ns"] = tmp["ns"]
      txt_tmp = tmp["next"]
    tmp = find_name_object(txt_tmp)
    if ("nameObject" in tmp):
      m_tmp["nameObject"] = tmp["nameObject"]
    txt_tmp = tmp["next"]

    lexer_list[cnt] = m_tmp
    cnt = cnt + 1
    return

  txt_idx = txt.find("LONG64 ")
  if (txt_idx > -1):
    txt_tmp = txt[txt_idx + len("LONG64 "):]
    m_tmp = {"type": "long64"}

    if (find_array(prev_txt)):
      raise BaseException("Implement long64 array " + prev_txt)

    if (find_attribute(prev_txt)==True):
      raise BaseException("Implement support for long64 attribute")

    m_tmp["isAttribute"] = False

    tmp = find_nullable(txt_tmp)
    m_tmp["nullable"] = tmp["nullable"]
    txt_tmp = tmp["next"]
    tmp = find_namespace(txt_tmp)
    if (tmp != None):
      m_tmp["name_ns"] = tmp["ns"]
      txt_tmp = tmp["next"]
    tmp = find_name_object(txt_tmp)
    if ("nameObject" in tmp):
      m_tmp["nameObject"] = tmp["nameObject"]
    txt_tmp = tmp["next"]

    lexer_list[cnt] = m_tmp
    cnt = cnt + 1
    return

  txt_idx = txt.find("double ")
  if (txt_idx > -1):
    txt_tmp = txt[txt_idx + len("double "):]
    m_tmp = {"type": "double"}

    if (find_array(prev_txt)):
      raise BaseException("Implement double array " + prev_txt)

    if (find_attribute(prev_txt)==True):
      raise BaseException("Implement support for double attribute")

    m_tmp["isAttribute"] = False

    tmp = find_nullable(txt_tmp)
    m_tmp["nullable"] = tmp["nullable"]
    txt_tmp = tmp["next"]
    tmp = find_namespace(txt_tmp)
    if (tmp != None):
      m_tmp["name_ns"] = tmp["ns"]
      txt_tmp = tmp["next"]
    tmp = find_name_object(txt_tmp)
    if ("nameObject" in tmp):
      m_tmp["nameObject"] = tmp["nameObject"]
    txt_tmp = tmp["next"]

    lexer_list[cnt] = m_tmp
    cnt = cnt + 1
    return

  if (unknown_lex(txt)):
    raise BaseException("Unknown lex: " + txt)

cnt = 0
prev_txt = None

for line in Lines:
  process_line(line, prev_txt)
  prev_txt = line

if (len(sys.argv) >= 2):
  e_type = sys.argv[1]
else:
  e_type = "W"

if (DEBUG):
  for it in lexer_list:
    print(lexer_list[it])

lexer_len = len(lexer_list)

i = 0

tmp = lexer_list[i]

main_ns = tmp["ns"]
main_object = tmp["typeObject"]

final_txt = "//struct " + main_ns + "__" + main_object + "\n"

if (e_type == "W"):
  final_txt += "WITSML_OBJECT_BEGIN(" + main_ns + ", " + main_object + ")\n"
elif (e_type == "A"):
  final_txt += "BSON_READ_ARRAY_OF_OBJECT21_BEGIN(" + main_ns + ", " + main_object + ")\n"
elif (e_type == "O"):
  final_txt += "BSON_READ_OBJECT21_BEGIN(" + main_ns + ", " + main_object + ")\n"
else:
  raise BaseException("Unknown parsing type " + e_type)

i = i + 1

attributes = []

last_one = lexer_len - 1

while (i < lexer_len):
  tmp = lexer_list[i]

  put_void = (last_one == i) and (len(attributes) == 0)

  if (tmp["isAttribute"] == False):
    final_txt += "  READ_" + e_type + "_"
    if (tmp["type"] == "object"):

      if (tmp["isArray"] == True):
        final_txt += "ARRAY_OF_"

      if (not put_void):
        final_txt += "OBJECT_21_OR_ELSE_GOTO_RESUME"
      else:
        final_txt += "OBJECT_21_VOID"

      if ("name_ns" in tmp):
        final_txt += "_B(" + tmp["name_ns"] + ", "
      else:
        final_txt += "("

      if "nameObject" in tmp:
        final_txt += main_object + ", " + tmp["nameObject"] + ", " + tmp["typeObject"] + ")\n"
      else:
        final_txt += main_object + ", " + "BUG" + ", " + tmp["typeObject"] + ")\n"

    elif (tmp["type"] == "string"):
      if (tmp["isArray"] == True):
        final_txt += "ARRAY_OF_"

      if (not put_void):
        final_txt += "UTF8_OBJECT_21_OR_ELSE_GOTO_RESUME"
      else:
        final_txt += "UTF8_OBJECT_21_VOID"

      if ("name_ns" in tmp):
        final_txt += "_B(" + tmp["name_ns"] + ", "
      else:
        final_txt += "("

      final_txt += main_object + ", " + tmp["nameObject"] + ")\n"

    elif (tmp["type"] == "boolean"):
      final_txt += "BOOLEAN_"

      if (tmp["nullable"] == True):
        final_txt += "NULLABLE_"

      if (not put_void):
        final_txt += "21_OR_ELSE_GOTO_RESUME"
      else:
        final_txt += "21_VOID"

      if ("name_ns" in tmp):
        final_txt += "_B(" + tmp["name_ns"] + ", "
      elif ("ns" in tmp):
        final_txt += "(" + tmp["ns"] + ", "
      else:
        final_txt += "("

      final_txt += main_object + ", " + tmp["nameObject"] + ")\n"

    elif (tmp["type"] == "time"):
      final_txt += "TIME_"

      if (tmp["nullable"] == True):
        final_txt += "NULLABLE_"

      if (not put_void):
        final_txt += "21_OR_ELSE_GOTO_RESUME"
      else:
        final_txt += "21_VOID"

      if ("name_ns" in tmp):
        final_txt += "_B(" + tmp["name_ns"] + ", "
      else:
        final_txt += "("

      final_txt += main_object + ", " + tmp["nameObject"] + ")\n"

    elif (tmp["type"] == "enum"):
      if (tmp["isArray"] == True):
        final_txt += "ARRAY_OF_OBJECT_ENUM_"

      #final_txt += "OBJECT_ENUM_"

      elif (tmp["nullable"] == True):
        final_txt += "OBJECT_ENUM_NULLABLE_"
      else:
        final_txt += "OBJECT_ENUM_"

      if (not put_void):
        final_txt += "21_OR_ELSE_GOTO_RESUME"
      else:
        final_txt += "21_VOID"

      if ("name_ns" in tmp):
        final_txt += "_B(" + tmp["name_ns"] + ", "
      else:
        final_txt += "(" + tmp["ns"] + ", "

      final_txt += main_object + ", " + tmp["nameObject"] + ", " + tmp["typeObject"] + ")\n"

    elif (tmp["type"] == "time"):
      final_txt += "TIME_"

      if (tmp["nullable"] == True):
        final_txt += "NULLABLE_"

      if (not put_void):
        final_txt += "21_OR_ELSE_GOTO_RESUME"
      else:
        final_txt += "21_VOID"

      if ("name_ns" in tmp):
        final_txt += "_B(" + tmp["name_ns"] + ", "
      else:
        final_txt += "("

      final_txt += main_object + ", " + tmp["nameObject"] + ")\n"

    elif (tmp["type"] == "long64"):
      final_txt += "LONG64_"

      if (tmp["nullable"] == True):
        final_txt += "NULLABLE_"

      if (not put_void):
        final_txt += "21_OR_ELSE_GOTO_RESUME"
      else:
        final_txt += "21_VOID"

      if ("name_ns" in tmp):
        final_txt += "_B(" + tmp["name_ns"] + ", "
      else:
        final_txt += "("

      final_txt += main_object + ", " + tmp["nameObject"] + ")\n"

    elif (tmp["type"] == "double"):
      final_txt += "DOUBLE_"

      if (tmp["nullable"] == True):
        final_txt += "NULLABLE_"

      if (not put_void):
        final_txt += "21_OR_ELSE_GOTO_RESUME"
      else:
        final_txt += "21_VOID"

      if ("name_ns" in tmp):
        final_txt += "_B(" + tmp["name_ns"] + ", "
      else:
        final_txt += "("

      final_txt += main_object + ", " + tmp["nameObject"] + ")\n"

  else:
    attributes.append(i)

  i = i + 1

attributes_size = len(attributes)
if (is_default_attribute(attributes, lexer_list) == True):
  final_txt += "  READ_" + e_type + "_PUT_DEFAULT_ATTRIBUTES_21_VOID(" + main_object + ")\n"
elif (attributes_size == 1):
  final_txt += "  READ_" + e_type + "_PUT_SINGLE_ATTR_21_VOID(" + main_object + ", " + lexer_list[attributes[0]]["nameObject"] +")\n"
elif (attributes_size == 2):
  final_txt += "  READ_" + e_type + "_PUT_TWO_ATTR_21_VOID(" + main_object + ", " + \
    lexer_list[attributes[0]]["nameObject"] + ", "+ lexer_list[attributes[1]]["nameObject"] +")\n"
elif (attributes_size > 2):
  final_txt += "  READ_" + e_type + "_PUT_MULTIPLE_ATTRIBUTES_21_VOID(" + main_object + ", SET_MULTIPLE_ATTRIBUTES("

  k = 0

  attributes_size = attributes_size - 1

  while (k < attributes_size):
    no = lexer_list[attributes[k]]["nameObject"]
    final_txt += "\n    CWS_CONST_BSON_KEY(\"" + no + "\"), " + main_object + "->" + no + ","
    k = k + 1

  no = lexer_list[attributes[k]]["nameObject"]
  final_txt += "\n    CWS_CONST_BSON_KEY(\"" + no + "\"), " + main_object + "->" + no + "\n  ))\n"

#  for i in attributes:
#    no = lexer_list[i]["nameObject"]
#    final_txt += "    CWS_CONST_BSON_KEY(\"" + no + "\"), " + main_object + "->" + no
#  raise BaseException("Not implemented attribute lexing")

if (e_type == "W"):
  final_txt += "WITSML_OBJECT_END(" + main_object + ")\n"
elif (e_type == "A"):
  final_txt += "BSON_READ_ARRAY_OF_OBJECT21_END(" + main_ns + ", " + main_object + ")\n"
elif (e_type == "O"):
  final_txt += "BSON_READ_OBJECT21_END(" + main_object + ")\n"

print(final_txt)


