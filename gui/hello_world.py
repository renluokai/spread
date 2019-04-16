import gi
gi.require_version('Gtk','3.0')
from gi.repository import Gtk
class MyWindow(Gtk.Window):
	def __init__(self):
		Gtk.Window.__init__(self, title='Hello World')
		self.button = Gtk.ColorButton(label='Click Here')
		self.button.connect('clicked',self.on_button_clicked)
		#self.add(self.button)
		self.combobox=Gtk.ComboBox()
		self.add(self.combobox)

	def on_button_clicked(self, widget):
		props = dir(widget.props)
		for i in props:
			print('{}={}'.format(i, widget.get_property(i)))

win = MyWindow()
win.connect('destroy',Gtk.main_quit)
win.show_all()
Gtk.main()
