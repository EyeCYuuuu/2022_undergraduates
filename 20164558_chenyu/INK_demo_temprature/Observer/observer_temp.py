""""
Intermittent System Demo: Temperature Observer
NOTE: must connect uart port first and configure uart port in the right mode
UART config: 115200
data type: 
    - sample log format: "!D??" (4bytes)
    - warning format: "$W??" (4bytes)
"""
import matplotlib
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import numpy as np
from tkinter import *
import time
import threading1
import serial

log_data = np.zeros([2,100], np.uint32)
log_curr_idx = 0
warn_data = np.zeros([2,100], np.uint32)
warn_curr_idx = 0
ser = serial.Serial('/dev/tty.usbmodem1303',115200,timeout=100)
ser.close()
ser_state = False

#更新数据
def update_log_info(info_lower, info_upper):
    global log_data
    global log_curr_idx
    log_data[0, log_curr_idx] = log_curr_idx
    log_data[1, log_curr_idx] = (info_upper*256+info_lower)/50
    log_curr_idx = log_curr_idx+1
    print(log_data[1,0:log_curr_idx])

#更新数据
def update_warn_info(info_lower, info_upper):
    global warn_data
    #global log_curr_idx
    global warn_curr_idx
    warn_data[0, warn_curr_idx] = warn_curr_idx
    warn_data[1, warn_curr_idx] = (info_upper*256+info_lower)/50
    warn_curr_idx = warn_curr_idx+1
    print(warn_data[0,0:warn_curr_idx])

#解释收到的数据包
def interpret_info(info_data, info_length):
    if info_length==4:
        print('check_info: The info is a package!')
        if((info_data[0]==ord('!')) and (info_data[1]==ord('D'))):
            print('check_info: this is a warning package.')
            update_log_info(info_data[2], info_data[3])
            return True
        elif((info_data[0]==ord('$')) and (info_data[1]==ord('W'))):
            print('check_info: this is a info package.')
            update_warn_info(info_data[2], info_data[3])
            return True
        else:
            print('check_info: info_data is not complete.')
            return False
    else:
        print('check_info: info_data is not complete.')
        return False


#更新单片机上传参数
def demoSerialReceiveData():
    while ser.is_open:
        time.sleep(0.05)
        if ser.in_waiting > 0:
            info_length = ser.in_waiting
            info_data = ser.read(info_length)        # read up to ten bytes (timeout)
            ser.reset_input_buffer()
            print(info_length)
            print(info_data)
            interpret_info(info_data, info_length)
        else:
            pass


#动态图像显示窗口
def video_loop():
    """
    动态matlib图表
    """
    global log_data
    global log_curr_idx
    global warn_data
    global warn_curr_idx

    fig_log.clf()
    fig_warning.clf()
    log = fig_log.add_subplot(111)
    log.plot(log_data[0,0:log_curr_idx],log_data[1,0:log_curr_idx])
    log.set_title('Temperature Log')
    canvas_log.draw()
    warning = fig_warning.add_subplot(111)
    warning.plot(warn_data[0,0:warn_curr_idx],warn_data[1,0:warn_curr_idx])
    warning.set_title('Temperature Warning')
    canvas_warning.draw()
    root.after(200, video_loop)

#-function: demoPlot
def demoPlot():
    global ser_state
    if (ser_state == False):
        ser.open()
        ser_state = True

        if ser.is_open:
            button1["text"] = "Disconnect"
            demoSerialThread = threading1.threading1(target = demoSerialReceiveData)
            demoSerialThread.setDaemon(True)
            demoSerialThread.start()
        else:
            print("串口没有打开")
    else:
        button1["text"] = "Connect"
        ser.close()
        ser_state = False




root = Tk()
root.title("Temperature Observer")
root.geometry('700x700')

"""
图像画布设置
"""
panel = Label(root)  # initialize image panel
panel.place(x=0,y=0,anchor='nw')
root.config(cursor="arrow")
name_log = Label(root,text = "Log",bg='#fafafa',font=('Arial', 24), width=9, height=1)
name_log.place(x=300,y=0,anchor='nw')
name_warning = Label(root,text = "Warning",bg='#fafafa',font=('Arial', 24), width=9, height=1)
name_warning.place(x=300,y=350,anchor='nw')

matplotlib.use('TkAgg')
fig_log = Figure(figsize=(7,3), dpi=100)
canvas_log = FigureCanvasTkAgg(fig_log, master=root)
canvas_log.draw()
canvas_log.get_tk_widget().place(x=0,y=35 ,anchor='nw')
fig_warning = Figure(figsize=(7,3), dpi=100)
canvas_warning = FigureCanvasTkAgg(fig_warning, master=root)
canvas_warning.draw()
canvas_warning.get_tk_widget().place(x=0,y=385 ,anchor='nw')

button1 = Button(root, text="Connect", command=demoPlot, width=10, height=1, bg='#fafafa')
button1.grid(row=1,column=1)

video_loop()
root.mainloop()