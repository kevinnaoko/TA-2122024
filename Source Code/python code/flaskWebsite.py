from hashlib import sha3_224
from re import search
from flask import Flask, redirect, url_for, render_template, request
import sqlite3
from dbHandler import dbManager 
import os
import math
from distutils import msvccompiler
import requests
from requests.structures import CaseInsensitiveDict
import json
from datetime import datetime
from dbTableInit import createDB
   
dbName =  "IoT.db"
dbLoc = "location.db"

#flask settings
app = Flask(__name__)
   
@app.route('/devices/<deviceSelect>')
def display(deviceSelect):  
    con = sqlite3.connect(dbName)
    cur = con.cursor()
    # reading all table names
    table_list = [a for a in cur.execute("SELECT name FROM sqlite_master WHERE type = 'table'")] 
     
    deviceName = (deviceSelect[0:7] + " " + deviceSelect[7]).capitalize() 

    querySelect = [ [0] for i in range (3) ]
    querySelect[0] = deviceSelect +"_s1"
    querySelect[1] = deviceSelect +"_s2"
    querySelect[2] = deviceSelect +"_device"

    lenIndex = [ [0] for i in range(3)]

    cur.execute("SELECT * FROM " + querySelect[0])
    lenIndex[0] = len(cur.fetchall())  

    cur.execute("SELECT * FROM " + querySelect[1])
    lenIndex[1] = len(cur.fetchall()) 

    s1data = [ [0]*(lenIndex[0]+1) for i in range(4)]
    s2data = [ [0]*(lenIndex[1]+1) for i in range(4)] 
    
    s1data[0][0] = 24
    s2data[0][0] = 24
    
    loopCount = 0
    for row in cur.execute("SELECT * FROM " + querySelect[0] + ' ORDER BY id DESC'):  
        print(row)
        s1data[0][loopCount] = row[1]
        s1data[1][loopCount] = row[2]
        s1data[2][loopCount] = row[3]
        s1data[3][loopCount] = row[4] 
        loopCount += 1
    
    loopCount = 0
    for row in cur.execute("SELECT * FROM " + querySelect[1] + ' ORDER BY id DESC'):  
        s2data[0][loopCount] = row[1]
        s2data[1][loopCount] = row[2]
        s2data[2][loopCount] = row[3]
        s2data[3][loopCount] = row[4] 
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

    # print(latestDeviceInfo)
    print(s2data[0])
    return render_template('displayOneDevice.html', len=lenIndex, s1_data=s1data, s2_data=s2data, device_info=latestDeviceInfo, device_name=deviceName, lat=lat, lon=lon)

@app.route("/home", methods=["POST", "GET"])
def home():
    # request handle
    if request.method == "POST":
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
            
            productAddId = int(user)
            
            stringS1 = 'charger' + str(productAddId) + '_s1'
            stringS2 = 'charger' + str(productAddId) + '_s2'
            stringDevice = 'charger' + str(productAddId) + '_device'
            createDB(stringS1, stringS2, stringDevice) 
            return redirect(url_for('home'))
        
        # elif request.form['submit_button'] == 'deleteIndexSubmit':
        #     print("DEL")
        #     return redirect(url_for('home'))
        
        
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
            # print("none") 
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
            # print("valid")  
            
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

@app.route("/")
def start():
    return redirect(url_for('home'))


@app.route("/remove-device/<deviceSelect>")
def removeDevice(deviceSelect):
    print("================================================================")
    print(deviceSelect)
    
    con = sqlite3.connect(dbName)
    cur = con.cursor()
    
    cur.execute('DROP TABLE ' + deviceSelect + '_s1')
    cur.execute('DROP TABLE ' + deviceSelect + '_s2')
    cur.execute('DROP TABLE ' + deviceSelect + '_device')
    
    cur.close()
    print('TABLE DROPPED')
    return redirect(url_for('home'))




@app.route("/admin/add-device")
def addDevice():
    return render_template('addDevice.html')

@app.route("/wip")
def wip():
    return "belum ada euy"

@app.route("/testMap")
def testmap(): 
    return render_template('map.html', lat=-6.876504, lng=107.612129)  

@app.route("/testTable")
def testtable(): 
    return render_template('testTable.html')  

if __name__ == "__main__":
    app.run(debug=True)
