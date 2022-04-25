
import hashlib
import sqlite3 
import os

print("ADD")
fname = "admin"
uname = "admin"
pword = "123"
cpword = "123"
print(fname)
print(uname)
print(pword)
print(cpword) 
    
iter_num = 100000
salt = os.urandom(32)
key = hashlib.pbkdf2_hmac(
    'sha256',
    pword.encode('utf-8'),
    salt,
    iter_num
)

print(salt)
print(key)

con = sqlite3.connect('user.db')
cur = con.cursor()

# row contents : id, fullname, username, pass(hash), salt
# index        :  0      1         2          3        4
cur.execute("SELECT * FROM userinfo WHERE username = ?", [uname,]) 
row = cur.fetchone()

if (row == None):  
    pass
else:
    if (uname == row[2]):
        print('Username unavailable, please try again') 

query = "INSERT INTO userinfo (fullname, username, password, salt) values (?,?,?,?)" 

cur.execute(query, [fname,uname,key,salt]) 
con.commit()

cur.close()
print('Register admin success') 