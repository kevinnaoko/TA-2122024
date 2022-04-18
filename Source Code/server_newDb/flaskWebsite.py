import paho.mqtt.client as mqtt
import hashlib
from re import search
from flask import Flask, redirect, url_for, render_template, request, flash, session
import sqlite3 
import os
import math  
from requests.structures import CaseInsensitiveDict
import json
from datetime import datetime
from dbTableInit import createDB 
   
dbName = "IoT.db"
dbUser = "user.db"

#flask settings
app = Flask(__name__)
app.secret_key = b'_5#y2L"F4Q8z\n\xec]/'
   
@app.route('/devices/<deviceSelect>')
def display(deviceSelect):  
    if "username" in session: 
        con = sqlite3.connect(dbName)
        cur = con.cursor()
        # reading all table names
        table_list = [a for a in cur.execute("SELECT name FROM sqlite_master WHERE type = 'table'")] 
        
        deviceName = (deviceSelect[0:7] + " " + deviceSelect[7:]).capitalize() 
        deviceSelection = deviceSelect[7:]
        print("TEST:",end='')
        print(deviceSelection)
        querySelect = [ [0] for i in range (3) ]
        querySelect[0] = "charger_s1"
        querySelect[1] = "charger_s2"
        querySelect[2] = "charger_device"

        lenIndex = [ [0] for i in range(3)]
        
        cur.execute("SELECT * FROM " + querySelect[0] + " WHERE productId = " + deviceSelection)
        lenIndex[0] = len(cur.fetchall())  

        cur.execute("SELECT * FROM " + querySelect[1] + " WHERE productId = " + deviceSelection)
        lenIndex[1] = len(cur.fetchall()) 

        # s1data = [ [0]*(7) for i in range(lenIndex[0]+1)]
        # s2data = [ [0]*(7) for i in range(lenIndex[1]+1)] 
        
        
        s1data = [ dict() for i in range(lenIndex[0]+1)]
        s2data = [ dict() for i in range(lenIndex[1]+1)] 
        
        
        
        # print("s1data")
        loopCount = 0
        # ['id', 'productId', 'Status', 'SN', 'Time', 'SoC', 'SoH', 'Volt']
        #    0        1           2       3      4      5      6       7
        for row in cur.execute("SELECT * FROM " + querySelect[0] + " WHERE productId = " + deviceSelection + ' ORDER BY id DESC'):  
            # print("HAI")
            # print(row)
            
            s1data[loopCount]['pid'] = row[1]
            s1data[loopCount]['stat'] = row[2]
            s1data[loopCount]['sn'] = row[3]
            s1data[loopCount]['time'] = row[4] 
            s1data[loopCount]['soc'] = row[5] 
            s1data[loopCount]['soh'] = row[6] 
            s1data[loopCount]['volt'] = row[7] 
            
            # print(s1data[loopCount])
            
            loopCount += 1
            
        
        # print("s2data")
        loopCount = 0
        # ['id', 'productId', 'Status', 'SN', 'Time', 'SoC', 'SoH', 'Volt']
        #    0        1           2       3      4      5      6       7
        for row in cur.execute("SELECT * FROM " + querySelect[1] + " WHERE productId = " + deviceSelection + ' ORDER BY id DESC'): 
            # print("HALO")
            # print(row) 
            
            s2data[loopCount]['pid'] = row[1]
            s2data[loopCount]['stat'] = row[2]
            s2data[loopCount]['sn'] = row[3]
            s2data[loopCount]['time'] = row[4] 
            s2data[loopCount]['soc'] = row[5] 
            s2data[loopCount]['soh'] = row[6] 
            s2data[loopCount]['volt'] = row[7] 
            
            # print(s2data[loopCount])
            loopCount += 1
        
        cur.execute('SELECT * FROM ' + querySelect[2] + ' ORDER BY id DESC LIMIT 1')
        latestDeviceInfo = cur.fetchone() 
        
        

        # Be sure to close the connection
        cur.close()
        con.close()  

        # get latitude longitude from CGI with geolocation API
        geoloc_url = "https://www.googleapis.com/geolocation/v1/geolocate?key=AIzaSyD62AEOgl0CjwL8dGWcl4fLG5IoiwHHghw"
        headers = CaseInsensitiveDict()
        headers["Content-Type"] = "application/json"  
        mcc=latestDeviceInfo[2]
        mnc=latestDeviceInfo[3]
        lac=latestDeviceInfo[4]
        cid=latestDeviceInfo[5]  

        api_payload = """
        { 
            "considerIp": true,
            "cellTowers": [
                {
                    "cellId": %s,
                    "locationAreaCode": %s,
                    "mobileCountryCode": %s,
                    "mobileNetworkCode": %s   
                }
            ]
        }
        """ %(cid, lac, mcc, mnc)

        # default lat lon kalau geolocApi dicomment
        lat= -6.8770328
        lon= 107.6123459
        
        # api request, comment dulu pas proses development
        # resp = requests.post(geoloc_url, headers=headers, data=api_payload)
        # print(resp.text)

        # pythonObj = json.loads(resp.text)  

        # lat = pythonObj['location']['lat']
        # lon = pythonObj['location']['lng']
        
        # print(lat)
        # print(lon)

        # print(latestDeviceInfo)
        # print(s2data[0])
        return render_template('displayOneDevice.html', len=lenIndex, s1_data=s1data, s2_data=s2data, device_info=latestDeviceInfo, device_name=deviceName, lat=lat, lon=lon)
    
    
    else:
        return redirect(url_for('login'))
    
@app.route("/home", methods=["POST", "GET"])
def home():
    if "username" in session: 
        # request handle
        if request.method == "POST":
            # button addIndex
            if request.form['submit_button'] == 'addIndexSubmit':
                print("ADD")
                user = request.form["addIndex"]
                print(user)
                
                con = sqlite3.connect(dbName)
                cur = con.cursor()
                
                # Get List of Tables:      
                tableListQuery = "SELECT name FROM sqlite_master WHERE type='table' ORDER BY Name"
                cur.execute(tableListQuery)
                tables = map(lambda t: t[0], cur.fetchall())

                # ['charger1_device', 'charger1_s1', 'charger1_s2', ... , 'sqlite_sequence']
                # jumlah tabel selalu 3 * n product + 1
                tableNames = list(tables)     
                currentProductCount = math.floor(len(tableNames)/3)                   
                 
                
                productId = "'charger" + user + "_s1'" 
                productAddId = int(user)
                # check if table exists 
                listOfTables = cur.execute( "SELECT name FROM sqlite_master WHERE type='table' AND name=" + productId+  ";").fetchall() 
                print(listOfTables)
                if listOfTables == []:
                    print('Table not found!')
                    stringS1 = 'charger' + str(productAddId) + '_s1'
                    stringS2 = 'charger' + str(productAddId) + '_s2'
                    stringDevice = 'charger' + str(productAddId) + '_device'
                    createDB(stringS1, stringS2, stringDevice)  
                    flash('Entry ' + str(productAddId) + ' added', 'success')
                    
                else:
                    print('Table found!') 
                    flash('Entry ' + str(productAddId) + ' exists, cannot add', 'danger')
                
            
                return redirect(url_for('home'))
            
            # button deleteIndex
            elif request.form['submit_button'] == 'deleteIndexSubmit':
                print("DEL")
                print("================================================================") 
                deviceSelect = request.form["deleteIndex"] 
                
                con = sqlite3.connect(dbName)
                cur = con.cursor()
                
                cur.execute('DROP TABLE ' + deviceSelect + '_s1')
                cur.execute('DROP TABLE ' + deviceSelect + '_s2')
                cur.execute('DROP TABLE ' + deviceSelect + '_device')
                
                cur.close()
                print('TABLE DROPPED')  
                
                flash('Charger ' + deviceSelect[7:] + ' deleted', 'success')
                return redirect(url_for('home'))
            
            
        else:
            pass
        
        # web info
        con = sqlite3.connect(dbName)
        cur = con.cursor()
        
        # sauce https://pagehalffull.wordpress.com/2012/11/14/python-script-to-count-tables-columns-and-rows-in-sqlite-database/ 
        
        # Get List of Tables:      
        tableListQuery = "SELECT name FROM sqlite_master WHERE type='table' ORDER BY Name"
        cur.execute(tableListQuery)
        tables = map(lambda t: t[0], cur.fetchall())

        # ['charger1_device', 'charger1_s1', 'charger1_s2', ... , 'sqlite_sequence']
        # jumlah tabel selalu 3 * n product + 1
        tableNames = list(tables)                       
        print(tableNames)
        
        # get   : charger ID, s1 stat, s1 soc, s2 stat, s2 soc, latest slot, latest time, chg mode
        # index :     0         1       2       3        4             5          6          7
        cols = 8
        productCount = math.floor(len(tableNames)/3) 
        print(productCount)
        latestData = [ [0]*cols for i in range(productCount)  ]

        index = 0 
        for i in range(len(tableNames)-1):      
            cur.execute('SELECT * FROM ' + tableNames[i] + ' ORDER BY id DESC LIMIT 1')
            lastRow = cur.fetchone()  
            
            if (lastRow == None): 
                if i % 3 == 0:
                    latestData[index][7] = '-' 
                    
                elif i % 3 == 1:
                    latestData[index][1] = '-'
                    latestData[index][2] = '-' 
                
                elif i % 3 == 2:
                    latestData[index][0] = tableNames[i].split("_")[0] 
                    latestData[index][3] = '-'
                    latestData[index][4] = '-'
                    latestData[index][5] = '-'
                    latestData[index][6] = '-' 
                    
                    index+=1
                    
            else:   
                # get chargerX_device   lastrow['id', 'chargeMode', 'MCC', 'MNC', 'LAC', 'CellID']
                if i % 3 == 0: 
                    latestData[index][7] = lastRow[1]
                    
                # get chargerX_s1       lastrow['id', 'Status', 'SN', 'Time', 'SoC']
                elif i % 3 == 1:
                    latestData[index][1] = lastRow[1] 
                    latestData[index][2] = lastRow[4] 
                    # latestData[index][2] = lastRow[3]
                    dateS1 = datetime.strptime(lastRow[3], '%y/%m/%d,%H:%M:%S')
                    
                # get chargerX_s2       lastrow['id', 'Status', 'SN', 'Time', 'SoC']
                elif i % 3 == 2:
                    latestData[index][0] = tableNames[i].split("_")[0] 
                    latestData[index][3] = lastRow[1]
                    latestData[index][4] = lastRow[4]   
                    dateS2 = datetime.strptime(lastRow[3], '%y/%m/%d,%H:%M:%S')
                    
                    if (dateS1>dateS2): 
                        latestData[index][5] = 'S1'
                        timestampStr = dateS1.strftime("%d-%B-%Y (%H:%M:%S)") 
                        
                    else: 
                        latestData[index][5] = 'S2'
                        timestampStr = dateS2.strftime("%d-%B-%Y (%H:%M:%S)") 
                        
                    latestData[index][6] = timestampStr 
                    index+=1
                
        print(latestData) 
        return render_template('displayGeneral.html', latest_data=latestData, product_count=productCount)
    
    else:
        return redirect(url_for('login'))

@app.route("/manage-users", methods=["POST", "GET"])
def manageUsers():
    if "username" in session: 
        # request handle
        if request.method == "POST":
            # button addIndex
            if request.form['submit_button'] == 'addUser':
                print("ADD")
                fname = request.form["fname"]
                uname = request.form["uname"]
                pword = request.form["pword"]
                cpword = request.form["conf_pword"]
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
                
                con = sqlite3.connect(dbUser)
                cur = con.cursor()
                
                # row contents : id, fullname, username, pass(hash), salt
                # index        :  0      1         2          3        4
                cur.execute("SELECT * FROM userinfo WHERE username = ?", (uname,)) 
                row = cur.fetchone()
                
                if (row == None):  
                    pass
                else:
                    if (uname == row[2]):
                        flash('Username unavailable, please try again', 'danger')
                        return redirect(url_for('manageUsers'))
                
                
                query = "INSERT INTO userinfo (fullname, username, password, salt) values (?,?,?,?)" 
                
                cur.execute(query, [fname,uname,key,salt]) 
                con.commit()
                
                cur.close()
                flash('Register success', 'success') 
                 
                return redirect(url_for('manageUsers'))
            
            # button deleteIndex
            elif request.form['submit_button'] == 'deleteUser':
                print("DEL")  
                userSelect = request.form["deleteUser"] 
                userSelect = "'" + userSelect + "'"
                print(userSelect)
                
                con = sqlite3.connect(dbUser)
                cur = con.cursor()
                # Deleting single record now
                sql_delete_query = """6"""
                cur.execute('DELETE from userinfo where username = ' + userSelect)
                con.commit()
                print("Record deleted successfully ")
                cur.close() 
                
                return redirect(url_for('manageUsers'))
             
        else:
            pass
        
        # web info
        con = sqlite3.connect(dbUser)
        cur = con.cursor()
          
        # userData   : num, fullname, username
        # index       :   0      1       2
        
        cur.execute('SELECT * FROM userinfo ORDER BY id')  
        cols = len(cur.fetchall())
        index = 2
        userData = [ [0]*cols for i in range(index)]
        loopCount = 0
        
        for row in cur.execute("SELECT * FROM userinfo ORDER BY id"):  
            print(row)
            userData[0][loopCount] = row[1]
            userData[1][loopCount] = row[2] 
            loopCount += 1

        
        return render_template('displayUsers.html', user_data=userData, user_count=loopCount)
    
    else:
        return redirect(url_for('login'))


@app.route("/")
def start():
    return redirect(url_for('login'))

# # obsolete
# @app.route("/remove-device/<deviceSelect>")
# def removeDevice(deviceSelect): 
#     print("================================================================")
#     print(deviceSelect)
    
#     con = sqlite3.connect(dbName)
#     cur = con.cursor()
    
#     cur.execute('DROP TABLE ' + deviceSelect + '_s1')
#     cur.execute('DROP TABLE ' + deviceSelect + '_s2')
#     cur.execute('DROP TABLE ' + deviceSelect + '_device')
    
#     cur.close()
#     print('TABLE DROPPED') 
#     return redirect(url_for('home'))


@app.route("/login", methods=["POST", "GET"])
def login(): 
    if request.method == "POST":
        if request.form['submit_btn'] == 'login':
            uname = request.form["username"]
            pword = request.form["password"] 
            print(uname)
            print(pword)
            
            con = sqlite3.connect(dbUser)
            cur = con.cursor()
            
            # row contents : id, fullname, username, pass(hash), salt(btn)
            # index        :  0      1         2          3        4
            cur.execute("SELECT * FROM userinfo WHERE username = ?", (uname,)) 
            row = cur.fetchone()
            
            if (row == None):
                flash('Invalid username / password. Please try again', 'danger') 
            
            else:
                iter_num = 100000
                salt = row[4]
                key = hashlib.pbkdf2_hmac(
                    'sha256',
                    pword.encode('utf-8'),
                    salt,
                    iter_num
                )
                
                if (key == row[3]):
                    flash('Login successful', 'success')
                    print("PASSWORD MATCH")
                    session['username'] = uname
                    # session['fullname'] = fname
                    # session['role']     = urole
                    
                    return redirect(url_for('home'))
                    
                else:
                    flash('Invalid username / password. Please try again', 'danger')
                    print("invalid uname.pw")
                    return redirect(url_for('login'))
                    
            return redirect(url_for('login'))
            
        
        elif request.form['submit_btn'] == 'register':
            print("REGIST")
            # uname = request.form["username"]
            # pword = request.form["password"] 
            # print(uname)
            # print(pword)
            
            # iter_num = 100000
            # salt = os.urandom(32)
            # key = hashlib.pbkdf2_hmac(
            #     'sha256',
            #     pword.encode('utf-8'),
            #     salt,
            #     iter_num
            # )
            
            # print(salt)
            # print(key)
            
            # con = sqlite3.connect(dbUser)
            # cur = con.cursor()
            
            # # row contents : id, fullname, username, pass(hash), salt
            # # index        :  0      1         2          3        4
            # cur.execute("SELECT * FROM userinfo WHERE username = ?", (uname,)) 
            # row = cur.fetchone()
            
            # if (row == None):  
            #     pass
            # else:
            #     if (uname == row[2]):
            #         flash('Username unavailable, please try again', 'danger')
            #         return redirect(url_for('login'))
             
            
            # query = "INSERT INTO userinfo (username, password, salt) values (?,?,?)" 
            
            # cur.execute(query, [uname,key,salt]) 
            # con.commit()
            
            # cur.close()
            
            
            
            flash('Cant register here', 'danger')
            return redirect(url_for('login'))
        
    else: 
        pass
        
    return render_template('login.html')

@app.route('/logout')
def logout():
    session.pop('username',None)
    return redirect(url_for('login'))

@app.route("/wip")
def wip():
    return "belum ada euy"

@app.route("/testMap")
def testmap(): 
    return render_template('map.html', lat=-6.876504, lng=107.612129)  

@app.route("/testTable")
def testtable(): 
    return render_template('testTable.html')  

@app.route("/testPublish")
def testPublish(): 
    
    broker_address = "34.101.49.52" 
    topic = "sys/commands"
    #broker_address="iot.eclipse.org"
    print("creating new instance")
    client = mqtt.Client("P1") #create new instance
    print("connecting to broker")
    client.connect(broker_address) #connect to broker 
    print("Publishing message to topic", topic)
    client.publish(topic,"testPaho")
    return render_template('testTable.html')  

if __name__ == "__main__":
    # app.run(host='192.168.1.102', port=5000, debug=True)
    app.run(debug=True)
