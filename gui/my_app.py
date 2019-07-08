import tkinter as tk
import tkinter.ttk as ttk
import appconfig
import appruntime

class MyApp():
	def __init__(master):
		my_runtim=appruntime.Runtime(master)
		my_config=appconfig.Runtime(master)
	def run():
		
		tk.mainloop()

if __name__=='':
	root=tk.Tk()
	my_app=MyApp(root)
	tk.mainloop()
