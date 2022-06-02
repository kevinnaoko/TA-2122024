from enum import unique
import paho.mqtt.publish as publish
import paho.mqtt.subscribe as subscribe
import hashlib
from re import I, search
from flask import Flask, redirect, url_for, render_template, request, flash, session
import sqlite3 
import os
import math  
from requests.structures import CaseInsensitiveDict
import json
from datetime import datetime
from dbTableInit import createDB 
import requests 
from multiprocessing import Process, Manager, Value
import time
from ctypes import c_char_p

# dbName = "IoT - Copy.db"
dbName = "IoT.db"
dbUser = "user.db"

mqttHost = "34.101.49.52"
mqttPort = 1883

#flask settings
app = Flask(__name__)
app.secret_key = b'_5#y2L"F4Q8z\n\xec]/'
   
@app.route('/devices/<deviceSelect>', methods=["POST", "GET"])
def display(deviceSelect):  
    if "username" in session: 
        if request.method == "POST":
            if request.form['submit_button'] == 'changeEnableSubmit':
                print("change status")
                print("================================================================") 
                deviceSelect = request.form["changeEnableIndex"] 
                
                con = sqlite3.connect(dbName)
                cur = con.cursor()  
                # ['id', 'productId', 'productEnable']
                #   0         1              2
                cur.execute("SELECT * FROM charger_enable WHERE productId=?" , (deviceSelect,)) 
                row = cur.fetchone()    
                
                if row[2] == 1:
                    setEn = 0
                    strFlash = ' disabled'
                else:
                    setEn = 1
                    strFlash = ' enabled'
                
                # publish command
                try:
                    commandTopic = "sys/charger" + str(deviceSelect) + "/commands" 
                    publish.single(commandTopic, setEn, hostname = mqttHost, port = mqttPort) 
                    
                except: 
                    flash('MQTT server unreachable. Can\'t change device status', 'warning')
                    params = "charger" + str(deviceSelect)
                    return redirect(url_for('display', deviceSelect=params))
                     
            #     # subscribe command
            # # try:
            #     currentTime = time.time()
            #     print("currenttime:{}".format(currentTime))
            #     while (time.time() - currentTime < 5): 
            #         commandTopicCallback = "sys/charger" + str(deviceSelect) + "/commandsCallback"  
            #         manager = Manager()
            #         string = manager.Value(c_char_p, "hehe")
            #         p = Process(target=customSubscribe, name="Foo", args=(commandTopicCallback, mqttHost, mqttPort, string))
            #         p.start()
                    
            #         # Wait 10 seconds for foo
            #         time.sleep(10)

            #         # Terminate foo
            #         p.terminate()

            #         # Cleanup
            #         p.join()    
                    
                    
            #         print ("OUTPUT PROCESS : {}".format(string.value))
                    
            #         outputMsg = ""
            #         # msg = subscribe(topics, hostname, 5)
            #         # print("%s %s" % (outputMsg.topic, outputMsg.payload))
            #         if outputMsg == "received": 
            #             flash('Charger ' + deviceSelect + strFlash, 'success') 
                        
            #         else:
            #             flash('Invalid client response. Can\'t change device status', 'warning')
            #             params = "charger" + str(deviceSelect)
            #             return redirect(url_for('display', deviceSelect=params))
                    
                # except: 
                #     flash('Client unreachable. Can\'t change device status', 'warning')
                #     params = "charger" + str(deviceSelect)
                #     return redirect(url_for('display', deviceSelect=params))
                
                query = "Update charger_enable set productEnable = ? where productId = ?"
                cur.execute(query,[setEn,deviceSelect])
                con.commit() 
                cur.close()
                print('info changed')       
                
                params = "charger" + str(deviceSelect)
                
                flash('Charger ' + deviceSelect + strFlash, 'success') 
                return redirect(url_for('display', deviceSelect=params))
                
            
            
        else:
            pass
        
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
        
        cur.execute("SELECT * FROM " + querySelect[0] + " WHERE productId = ?", (deviceSelection,))
        lenIndex[0] = len(cur.fetchall())  

        cur.execute("SELECT * FROM " + querySelect[1] + " WHERE productId = ?", (deviceSelection,))
        lenIndex[1] = len(cur.fetchall()) 
        
        s1data = [ dict() for i in range(lenIndex[0]+1)]
        s2data = [ dict() for i in range(lenIndex[1]+1)] 
        latestInfo = dict() 
        
        # get charger_s1 data
        loopCount = 0
        # ['id', 'productId', 'Status', 'SN', 'Time', 'SoC', 'SoH', 'Volt']
        #    0        1           2       3      4      5      6       7
        for row in cur.execute("SELECT * FROM " + querySelect[0] + " WHERE productId = ? ORDER BY id DESC", (deviceSelection,)):  
            if row[2] == -1:
                continue
            s1data[loopCount]['pid'] = row[1]
            s1data[loopCount]['stat'] = row[2]
            s1data[loopCount]['sn'] = row[3]
            s1data[loopCount]['time'] = row[4] 
            s1data[loopCount]['soc'] = row[5] 
            s1data[loopCount]['soh'] = row[6] 
            s1data[loopCount]['volt'] = row[7] 
            
            loopCount += 1
        
        # get charger_s2 data
        loopCount = 0
        # ['id', 'productId', 'Status', 'SN', 'Time', 'SoC', 'SoH', 'Volt']
        #    0        1           2       3      4      5      6       7
        for row in cur.execute("SELECT * FROM " + querySelect[1] + " WHERE productId = ? ORDER BY id DESC", (deviceSelection,)): 
            # print("HALO")
            # print(row) 
            if row[2] == -1:
                continue
            s2data[loopCount]['pid'] = row[1]
            s2data[loopCount]['stat'] = row[2]
            s2data[loopCount]['sn'] = row[3]
            s2data[loopCount]['time'] = row[4] 
            s2data[loopCount]['soc'] = row[5] 
            s2data[loopCount]['soh'] = row[6] 
            s2data[loopCount]['volt'] = row[7] 
            
            # print(s2data[loopCount])
            loopCount += 1
        
        # get charger_device data
        # ['id', 'productId', 'chargeMode', 'MCC', 'MNC', 'LAC', 'CellID']
        #    0        1           2           3      4      5       6      
        cur.execute('SELECT * FROM ' + querySelect[2] + " WHERE productId = ? ORDER BY id DESC LIMIT 1", (deviceSelection,))
        row = cur.fetchone() 
        print('latestinfo')
        latestInfo['pid'] = row[1]
        latestInfo['chgMode'] = row[2]
        latestInfo['mcc'] = row[3]
        latestInfo['mnc'] = row[4]
        latestInfo['lac'] = row[5]
        latestInfo['cid'] = row[6]
        
        print(latestInfo)
        
        # get charger status
        # cek apakah sudah ada data di charger_enable
        # kalau belum, insert data lalu inisialisasi dengan value 1 (ON)
        # kalau sudah, ambil info device_enable
        # ['id', 'productId', 'productEnable']
        #   0         1              2
        cur.execute('SELECT * FROM charger_enable WHERE productId = ?', [str(deviceSelection),]) 
        row = cur.fetchone() 
        deviceEn = row[2]  

        cur.close()
        con.close()  

        # get latitude longitude from CGI with geolocation API
        geoloc_url = "https://www.googleapis.com/geolocation/v1/geolocate?key=AIzaSyD62AEOgl0CjwL8dGWcl4fLG5IoiwHHghw"
        headers = CaseInsensitiveDict()
        headers["Content-Type"] = "application/json"  
        mcc=latestInfo['mcc']
        mnc=latestInfo['mnc']
        lac=latestInfo['lac']
        cid=latestInfo['cid']

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
        resp = requests.post(geoloc_url, headers=headers, data=api_payload)
        print(resp.text)

        pythonObj = json.loads(resp.text)  

        lat = pythonObj['location']['lat']
        lon = pythonObj['location']['lng']
        
        print(lat)
        print(lon)

        # print(s2data[0])
        return render_template('displayOneDevice.html', len=lenIndex, s1_data=s1data, s2_data=s2data, device_info=latestInfo, device_id=deviceSelection, lat=lat, lon=lon, device_en = deviceEn)
    
    
    else:
        return redirect(url_for('login'))
    
@app.route("/home", methods=["POST", "GET"])
def home():
    if "username" in session: 
        # request handle
        if request.method == "POST":  
            # button deleteIndex
            if request.form['submit_button'] == 'deleteIndexSubmit':
                print("DEL")
                print("================================================================") 
                deviceSelect = request.form["deleteIndex"] 
                
                con = sqlite3.connect(dbName)
                cur = con.cursor()  
                cur.execute("DELETE FROM charger_s1 WHERE productId=?" , (deviceSelect,))
                cur.execute("DELETE FROM charger_s2 WHERE productId=?" , (deviceSelect,))
                cur.execute("DELETE FROM charger_device WHERE productId=?" , (deviceSelect,))
                con.commit()
                cur.close()
                print('index dropped')  
                
                flash('Charger ' + deviceSelect + ' deleted', 'success')
                return redirect(url_for('home'))
            
            # button change on/off
            if request.form['submit_button'] == 'changeEnableSubmit':
                print("change status")
                print("================================================================") 
                deviceSelect = request.form["changeEnableIndex"] 
                
                con = sqlite3.connect(dbName)
                cur = con.cursor()  
                # ['id', 'productId', 'productEnable']
                #   0         1              2
                cur.execute("SELECT * FROM charger_enable WHERE productId=?" , (deviceSelect,)) 
                row = cur.fetchone()    
                
                if row[2] == 1:
                    setEn = 0
                    strFlash = ' disabled'
                else:
                    setEn = 1
                    strFlash = ' enabled'
                    
                query = "Update charger_enable set productEnable = ? where productId = ?"
                cur.execute(query,[setEn,deviceSelect])
                con.commit() 
                cur.close()
                print('info changed')  
                
                # publish command
                try:
                    commandTopic = "sys/charger" + str(deviceSelect) + "/commands" 
                    publish.single(commandTopic, setEn, hostname=mqttHost, port = mqttPort)
                    
                    flash('Charger ' + deviceSelect + strFlash, 'success')
                
                except: 
                    flash('MQTT server unreachable. Failed to change device status', 'warning')
                
                return redirect(url_for('home'))
            
            
        else:
            pass
        
        con = sqlite3.connect(dbName)
        cur = con.cursor()
        
        # sauce https://pagehalffull.wordpress.com/2012/11/14/python-script-to-count-tables-columns-and-rows-in-sqlite-database/ 
        
        # Get unique pids:       
        uniqueIDs1 = []
        uniqueIDs2 = []
        uniqueIDdev = []
        
        for row in cur.execute("SELECT DISTINCT productId FROM charger_s1 ORDER BY productId" ):
            # print (row)
            uniqueIDs1.append(row[0])
            
        for row in cur.execute("SELECT DISTINCT productId FROM charger_s2 ORDER BY productId" ):
            # print (row)
            uniqueIDs2.append(row[0])
            
        for row in cur.execute("SELECT DISTINCT productId FROM charger_device ORDER BY productId" ):
            # print (row)
            uniqueIDdev.append(row[0])
        
        combineIDs = uniqueIDs1 + uniqueIDs2 + uniqueIDdev 
        list_set = set(combineIDs) 
        uniqueIDs = (list(list_set))
        
        print('uid')
        print(uniqueIDs)
        
        productCount = len(uniqueIDs)
        print(productCount)
        latestData = [ dict() for i in range(len(uniqueIDs))  ]
 
        for i in range(len(uniqueIDs)): 
            
            # cek apakah sudah ada data di charger_enable
            # kalau belum, insert data lalu inisialisasi dengan value 1 (ON)
            # kalau sudah, ambil info device_enable
            # ['id', 'productId', 'productEnable']
            #   0         1              2
            cur.execute('SELECT * FROM charger_enable WHERE productId = ?', [str(uniqueIDs[i]),]) 
            lastRow = cur.fetchone()
            
            if (lastRow == None):
                query = "INSERT INTO charger_enable (productId, productEnable) values (?,?)" 
                cur.execute(query, [uniqueIDs[i],1])  
                con.commit()
                latestData[i]['enable'] = 1
            else:
                latestData[i]['enable'] = lastRow[2] 
            
            latestData[i]['pid'] = uniqueIDs[i] 
            # ambil latest row dari charger_s1 produk ke i
            # ['id', 'productId', 'Status', 'SN', 'Time', 'SoC', 'SoH', 'Volt']
            #    0        1           2       3      4      5      6       7
            cur.execute('SELECT * FROM charger_s1 WHERE productId = ? ORDER BY id DESC LIMIT 1', [str(uniqueIDs[i]),])
            lastRow = cur.fetchone()
            
            if (lastRow == None): 
                latestData[i]['s1_stat'] = '-'
                latestData[i]['s1_soc'] = '-'
                latestData[i]['s1_latest'] = '-'
            
            else:
                latestData[i]['s1_stat'] = lastRow[2]
                latestData[i]['s1_soc'] = lastRow[5]
                latestData[i]['s1_latest'] = lastRow[4]
            
            # ambil latest row dari charger_s2 produk ke i
            # ['id', 'productId', 'Status', 'SN', 'Time', 'SoC', 'SoH', 'Volt']
            #    0        1           2       3      4      5      6       7
            cur.execute('SELECT * FROM charger_s2 WHERE productId = ? ORDER BY id DESC LIMIT 1',[str(uniqueIDs[i]),])
            lastRow = cur.fetchone()
            
            if (lastRow == None): 
                latestData[i]['s2_stat'] = '-'
                latestData[i]['s2_soc'] = '-'
                latestData[i]['s2_latest'] = '-'
            
            else:
                latestData[i]['s2_stat'] = lastRow[2]
                latestData[i]['s2_soc'] = lastRow[5]
                latestData[i]['s2_latest'] = lastRow[4]
            
            # ambil latest row dari charger_device produk ke i
            # ['id', 'productId', 'chargeMode', 'MCC', 'MNC', 'LAC', 'CellID']
            #    0        1           2           3      4      5       6    
            cur.execute('SELECT * FROM charger_device WHERE productId = ? ORDER BY id DESC LIMIT 1', [str(uniqueIDs[i]),])
            lastRow = cur.fetchone()
            
            if (lastRow == None): 
                latestData[i]['chgMode'] = '-'
            
            else:
                latestData[i]['chgMode'] = lastRow[2]
                
            # proses tanggal
            try:
                dateS1 = datetime.strptime(latestData[i]['s1_latest'], '%y/%m/%d,%H:%M:%S')
                dateS2 = datetime.strptime(latestData[i]['s2_latest'], '%y/%m/%d,%H:%M:%S')
                if (dateS1>dateS2): 
                    latestData[i]['latestSlot'] = 'S1'
                    timestampStr = dateS1.strftime("%d-%B-%Y (%H:%M:%S)") 
                    
                else: 
                    latestData[i]['latestSlot'] = 'S2'
                    timestampStr = dateS2.strftime("%d-%B-%Y (%H:%M:%S)") 
                    
                latestData[i]['latestTime'] = timestampStr 
                
            except:
                latestData[i]['latestSlot'] = '-'
                latestData[i]['latestTime'] = '-' 
             
        
        for x in latestData:
            print(x)
         
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
                
                if (pword != cpword):
                    flash('Password not match, please try again', 'danger')
                    return redirect(url_for('manageUsers'))
                 
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
                cur.execute("SELECT * FROM userinfo WHERE username = ?", [uname,]) 
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
                print(userSelect)
                
                con = sqlite3.connect(dbUser)
                cur = con.cursor()
                cur.execute('DELETE from userinfo where username = ?', [userSelect,])
                con.commit()
                print("Record deleted successfully ")
                flash('Entry deleted successfully', 'danger')
                cur.close() 
                
                return redirect(url_for('manageUsers'))
            
            # button editIndex
            elif request.form['submit_button'] == 'editUser':
                print("EDIT")  
                userSelect = request.form["userSelection"] 
                
                con = sqlite3.connect(dbUser)
                cur = con.cursor()
                
                fname = request.form["fname"]
                uname = request.form["uname"]
                pword = request.form["pword"]
                cpword = request.form["conf_pword"]
                
                print(fname)
                print(uname)
                print(pword)
                print(cpword)
                
                if (pword != cpword):
                    flash('Password not match, please try again', 'danger')
                    return redirect(url_for('manageUsers'))
                
                
                print(userSelect)
                print('')
                
                if (bool(pword) == False):
                    print("NO PWD")
                    
                    query = "UPDATE userinfo SET fullname=?, username=? WHERE username=? " 
                    cur.execute(query, [fname,uname,userSelect]) 
                    con.commit()
                    
                    cur.close()
                    flash('update data success', 'success') 
                    
                else:
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
                
                    query = "UPDATE userinfo SET fullname=?, username=?, password=?, salt=? WHERE username=? " 
                    cur.execute(query, [fname,uname,key,salt,userSelect]) 
                    con.commit()
                    
                    cur.close()
                    flash('update data success', 'success') 
                
                return redirect(url_for('manageUsers'))
             
        else:
            pass
        
        # connect db
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

@app.route("/batteries", methods=["POST", "GET"])
def batteries():
    if "username" in session: 
        # request handle
        if request.method == "POST":  
            pass
            
        else:
            pass
        
        con = sqlite3.connect(dbName)
        cur = con.cursor()
        
        # sauce https://pagehalffull.wordpress.com/2012/11/14/python-script-to-count-tables-columns-and-rows-in-sqlite-database/ 
        
        # Get unique pids:       
        battSNs1 = []
        battSNs2 = [] 
        
        for row in cur.execute("SELECT DISTINCT SN FROM charger_s1" ):
            # print (row)
            battSNs1.append(row[0])
            
        for row in cur.execute("SELECT DISTINCT SN FROM charger_s2 " ):
            # print (row)
            battSNs2.append(row[0]) 
        
        combineSNs = battSNs1 + battSNs2 
        list_set = set(combineSNs) 
        uniqueSNs = (list(list_set))
        
        uniqueSNs = [x for x in uniqueSNs if x != '']
        
        print('usn')
        print(uniqueSNs)
        
        battCount = len(uniqueSNs)
        print(battCount)
        
        latestData = [ dict() for i in range(len(uniqueSNs))  ]
 
        for i in range(len(uniqueSNs)): 
             
            
            latestData[i]['battSN'] = uniqueSNs[i] 
            # ambil latest row dari charger_s1 produk ke i
            # ['id', 'productId', 'Status', 'SN', 'Time', 'SoC', 'SoH', 'Volt']
            #    0        1           2       3      4      5      6       7
            cur.execute('SELECT * FROM charger_s1 WHERE SN = ? ORDER BY id DESC LIMIT 1', (str(uniqueSNs[i]),))
            lastRow = cur.fetchone()
            
            if (lastRow == None): 
                latestData[i]['s1_sn'] = '-'
                latestData[i]['s1_sn_latest'] = '-'
                latestData[i]['s1_sn_stat'] = '-'
            
            else:
                latestData[i]['s1_sn'] = lastRow[3]
                latestData[i]['s1_sn_latest'] = lastRow[4]
                latestData[i]['s1_sn_stat'] = lastRow[2] 
            
            # ambil latest row dari charger_s2 produk ke i
            # ['id', 'productId', 'Status', 'SN', 'Time', 'SoC', 'SoH', 'Volt']
            #    0        1           2       3      4      5      6       7
            cur.execute('SELECT * FROM charger_s2 WHERE SN = ? ORDER BY id DESC LIMIT 1', (str(uniqueSNs[i]),)) 
            lastRow = cur.fetchone()
            
            if (lastRow == None): 
                latestData[i]['s2_sn'] = '-'
                latestData[i]['s2_sn_latest'] = '-'
                latestData[i]['s2_sn_stat'] = '-'
            
            else:
                latestData[i]['s2_sn'] = lastRow[3]
                latestData[i]['s2_sn_latest'] = lastRow[4]
                latestData[i]['s2_sn_stat'] = lastRow[2]  
                
            # proses slot mana yang lebih latest
            try:
                dateS1 = datetime.strptime(latestData[i]['s1_sn_latest'], '%y/%m/%d,%H:%M:%S')
                dateS2 = datetime.strptime(latestData[i]['s1_sn_latest'], '%y/%m/%d,%H:%M:%S')
                
                if (dateS1>dateS2): 
                    latestData[i]['battSN_latestSlot'] = 'S1'
                    latestData[i]['battSN_status'] = latestData[i]['s1_sn_stat']
                    timestampStr = dateS1.strftime("%d-%B-%Y (%H:%M:%S)") 
                    
                else: 
                    latestData[i]['battSN_latestSlot'] = 'S2'
                    latestData[i]['battSN_status'] = latestData[i]['s2_sn_stat']
                    timestampStr = dateS2.strftime("%d-%B-%Y (%H:%M:%S)") 
                    
                latestData[i]['battSN_latestTime'] = timestampStr 
            
            
            #asumsi slot1 yang paling latest 
            except:
                latestData[i]['battSN_latestSlot'] = 'S1'
                latestData[i]['battSN_latestTime'] = 'N/A' 
             
        
        for x in latestData:
            print(x)
         
        return render_template('displayBatteries.html', latest_data=latestData, product_count=battCount)
    
    else:
        return redirect(url_for('login'))



@app.route('/connectedBatteries/<battSN>')
def singleBattery(battSN):  
    if "username" in session: 
        con = sqlite3.connect(dbName)
        cur = con.cursor()
        # reading all table names
        table_list = [a for a in cur.execute("SELECT name FROM sqlite_master WHERE type = 'table'")] 
        
        batteryName = "Battery #" + str(battSN) 
        

        lenIndex = [ [0] for i in range(2)]
        
        cur.execute("SELECT * FROM charger_s1 WHERE SN = ?", (battSN,))
        lenIndex[0] = len(cur.fetchall())   

        cur.execute("SELECT * FROM charger_s2 WHERE SN = ?", (battSN,))
        lenIndex[1] = len(cur.fetchall()) 
        
        s1data = [ dict() for i in range(lenIndex[0]+1)]
        s2data = [ dict() for i in range(lenIndex[1]+1)] 
        battData = [ dict() for i in range(lenIndex[0]+lenIndex[1])] 
         
        
        # get SN data in slot 1 (if available)
        loopCount = 0
        # ['id', 'productId', 'Status', 'SN', 'Time', 'SoC', 'SoH', 'Volt', 'ChargeMode']
        #    0        1           2       3      4      5      6       7         8
        for row in cur.execute("SELECT * FROM charger_s1 WHERE SN = ?", (battSN,)):
            s1data[loopCount]['stat'] = row[2]
            s1data[loopCount]['time'] = row[4] 
            s1data[loopCount]['soc'] = row[5] 
            s1data[loopCount]['soh'] = row[6] 
            s1data[loopCount]['volt'] = row[7] 
            s1data[loopCount]['chargeMode'] = row[8] 
            
            loopCount += 1
        
        # get SN data in slot 2 (if available)
        loopCount = 0 
        # ['id', 'productId', 'Status', 'SN', 'Time', 'SoC', 'SoH', 'Volt', 'ChargeMode']
        #    0        1           2       3      4      5      6       7         8
        for row in cur.execute("SELECT * FROM charger_s2 WHERE SN = ?", (battSN,)):      
            s2data[loopCount]['stat'] = row[2] 
            s2data[loopCount]['time'] = row[4] 
            s2data[loopCount]['soc'] = row[5] 
            s2data[loopCount]['soh'] = row[6] 
            s2data[loopCount]['volt'] = row[7] 
            s2data[loopCount]['chargeMode'] = row[8] 
            
            # print(s2data[loopCount])
            loopCount += 1
        
         
        validChargingCycle = 0
        invalidChargingCycle = 0
        
        for i in s1data:
            print(i)
            
        # process data valid per 1 charging cycle di slot 1
        for i in range(lenIndex[0]-1):  
            space = 1
            # syarat kondisi 1 charging cycle
            # setelah stat nya 1, setelahnya harus langsung 2 atau 1  
            if s1data[i]['stat'] == 1: 
                # kalau 1, berarti lagi ngubah mode pengisian daya, jadi ditraverse sampai ketemu 2
                if s1data[i+1]['stat'] == 1:
                    chargeCondition = 'Partial'
                elif s1data[i+1]['stat'] == 2: 
                    chargeCondition = 'Full' 
                elif s1data[i+1]['stat'] == 0:
                    chargeCondition = 'Partial'
                elif s1data[i+1]['stat'] == -1:
                    invalidChargingCycle+=1
                    continue
                
                battData[validChargingCycle]['condition'] = chargeCondition
                battData[validChargingCycle]['init_soc']  = s1data[i]['soc']
                battData[validChargingCycle]['init_soh']  = s1data[i]['soh']
                battData[validChargingCycle]['init_volt'] = s1data[i]['volt']
                battData[validChargingCycle]['init_time'] = s1data[i]['time']
                battData[validChargingCycle]['chg_mode']  = s1data[i]['chargeMode']
                battData[validChargingCycle]['fin_soc']   = s1data[i+space]['soc']
                battData[validChargingCycle]['fin_soh']   = s1data[i+space]['soh']
                battData[validChargingCycle]['fin_volt']  = s1data[i+space]['volt']
                battData[validChargingCycle]['fin_time']  = s1data[i+space]['time']
                validChargingCycle += 1
                    
        # process data valid per 1 charging cycle di slot 2
        for i in range(lenIndex[1]-1): 
            exit = 0
            space = 1
            # syarat kondisi 1 charging cycle
            # setelah stat nya 1, setelahnya harus langsung 2 atau 1  
            if s2data[i]['stat'] == 1:  
                
                if s2data[i+1]['stat'] == 1:
                    chargeCondition = 'Partial'
                elif s2data[i+1]['stat'] == 2: 
                    chargeCondition = 'Full' 
                elif s2data[i+1]['stat'] == 0:
                    chargeCondition = 'Partial'
                elif s2data[i+1]['stat'] == -1:
                    invalidChargingCycle+=1
                    continue
                  
                battData[validChargingCycle]['condition'] = chargeCondition
                battData[validChargingCycle]['init_soc']  = s2data[i]['soc']
                battData[validChargingCycle]['init_soh']  = s2data[i]['soh']
                battData[validChargingCycle]['init_volt'] = s2data[i]['volt']
                battData[validChargingCycle]['init_time'] = s2data[i]['time']
                battData[validChargingCycle]['chg_mode']  = s2data[i]['chargeMode']
                battData[validChargingCycle]['fin_soc']   = s2data[i+space]['soc']
                battData[validChargingCycle]['fin_soh']   = s2data[i+space]['soh']
                battData[validChargingCycle]['fin_volt']  = s2data[i+space]['volt']
                battData[validChargingCycle]['fin_time']  = s2data[i+space]['time']
                validChargingCycle += 1
        
        # print(battData[0])
        # proses tanggal, durasi, delta SOC
        for i in range(validChargingCycle):
            battData[i]['delta_soc'] = battData[i]['fin_soc'] - battData[i]['init_soc']
            try:
                dateStart = datetime.strptime(battData[i]['init_time'], '%y/%m/%d,%H:%M:%S')
                dateFinish = datetime.strptime(battData[i]['fin_time'], '%y/%m/%d,%H:%M:%S')
                
                duration = (dateFinish - dateStart).total_seconds()
                if (duration < 3600):
                    durationString = "{:.0f} mins".format((duration % 3600) / 60)  
                else:
                    durationString = "{} hrs {:.0f} mins".format(math.floor(duration / 3600), (duration % 3600) / 60)  
                battData[i]['delta_time'] = durationString
                
                battData[i]['init_time'] = dateStart.strftime("%d-%B-%Y (%H:%M:%S)") 
                battData[i]['fin_time'] = dateFinish.strftime("%d-%B-%Y (%H:%M:%S)") 
                
            except:
                battData[i]['init_time'] = '-'
                battData[i]['fin_time'] = '-' 
                battData[i]['delta_time'] = '-' 
                 
        cur.close()
        con.close()  
        
        print("inv:{}".format(invalidChargingCycle))
 
        return render_template('displayOneBattery.html', lenValid=validChargingCycle, lenInvalid=invalidChargingCycle, batt_data=battData, batt_sn=battSN)
    
    
    else:
        return redirect(url_for('login'))

@app.route("/")
def start():
    return redirect(url_for('login'))


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
                    # flash('Login successful', 'success')
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
            # uncomment buat regist kalau lupa pwd
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

# other functions 
def customSubscribe(topic, host, port, ret_value):
    msg = subscribe.simple(topic, hostname = host, port = port)
    print("MESSAGE PAYLOAD = {}".format(msg.payload))
    ret_value.value = msg.payload  

if __name__ == "__main__":
    # app.run(host='192.168.0.114', port=5000, debug=True)
    app.run(debug=True)
