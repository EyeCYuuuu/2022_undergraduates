import threading
 
class threading1(threading.Thread):
 
  def __init__(self, *args, **kwargs):
    super(threading1, self).__init__(*args, **kwargs)
    self.__flag = threading.Event()   # 用于暂停线程的标识
    self.__flag.set()    # 设置为True
    self.__running = threading.Event()   # 用于停止线程的标识
    self.__running.set()   # 将running设置为True

  def pause(self):
    self.__flag.clear()   # 设置为False, 让线程阻塞
 
  def resume(self):
    self.__flag.set()  # 设置为True, 让线程停止阻塞
 
  def stop(self):
    self.__flag.set()    # 将线程从暂停状态恢复, 如何已经暂停的话
    self.__running.clear()    # 设置为False 