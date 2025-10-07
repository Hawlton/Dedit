#include <fstream>
#include <Windows.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>

using namespace std;


struct LineBuffer
{
	char* data;
	int size;
	int gap_start;
	int gap_end;
	int line_length;


	int gap_size() const
	{
		return gap_end - gap_start;
	}

	int cursor_pos_to_buffer_index(int cursor_pos)
	{
		if (cursor_pos < gap_start)
		{
			return cursor_pos;
		}
		return cursor_pos + gap_size();
	}

	void expand_buffer(int new_size)
	{
		if (new_size <= size) return;
		char* new_data = new char[new_size];
		memcpy(new_data, data, gap_start);

		//Moves all text data after cursor to end of buffer and sets gap end to the new buffer - the length of the trailing text data
		int length_after_gap = size - gap_end;
		int new_gap_end = new_size - length_after_gap;
		memcpy(new_data + new_gap_end, data + gap_end, length_after_gap);

		delete[] data;
		data = new_data;
		size = new_size;
		gap_end = new_gap_end;


	}

	void move_gap(int new_cursor_pos)
	{
		new_cursor_pos = min(new_cursor_pos, line_length);
		new_cursor_pos = max(0, new_cursor_pos);

		if (new_cursor_pos == gap_start) return;
		if (new_cursor_pos < gap_start)
		{
			int count = gap_start - new_cursor_pos;
			while (count--)
			{
				gap_start--;
				gap_end--;
				data[gap_end] = data[gap_start];
			}
		}
		else
		{
			int count = new_cursor_pos - gap_start;
			while (count--)
			{
				data[gap_start] = data[gap_end];
				gap_start++;
				gap_end++;
			}

		}
	}

	void insert_character(char c)
	{
		if (gap_start == gap_end)
		{
			expand_buffer(size * 2);
		}
		data[gap_start] = c;
		gap_start++;
		line_length++;
	}

	bool backspace_button()
	{
		if (gap_start == 0) return false;
		gap_start--;
		line_length--;
		return true;
	}

	bool delete_button()
	{
		int tail = line_length - gap_start;
		if (tail == 0) return false;
		gap_end++;
		line_length--;
		return true;
	}

	void draw_line() const
	{
		if (gap_start > 0)
		{
			cout.write(data, gap_start);
		}
		int tail = line_length - gap_start;
		if (tail > 0)
		{
			cout.write(data + gap_end, tail);
		}
	}

};

struct TextFile
{
	vector<LineBuffer*> lines;
	int cursor_row;
	int cursor_col;
};

void init_text_file(TextFile* tf)
{
	tf->cursor_col = 0;
	tf->cursor_row = 0;
}

LineBuffer* create_line_buffer()
{
	LineBuffer* line = new LineBuffer;
	line->size = 128;
	line->gap_start = 0;
	line->gap_end = line->size;
	line->line_length = 0;

	line->data = new char[line->size];
	return line;

}

void redraw_line(HANDLE out, LineBuffer* line, SHORT row, int& last_drawn_length)
{
	COORD redraw_pos = { 0, row };
	SetConsoleCursorPosition(out, redraw_pos);
	line->draw_line();
	if (line->line_length < last_drawn_length)
	{
		//clear the rest of the line if it got shorter
		for (int i = line->line_length; i < last_drawn_length; i++)
		{
			cout << ' ';
		}
		SetConsoleCursorPosition(out, { (SHORT)line->line_length, row });
	}
	last_drawn_length = line->line_length;
	COORD carrot = { (SHORT)line->gap_start, row };
	SetConsoleCursorPosition(out, carrot);
	cout.flush();
}

void insert_new_line(TextFile* tf, int current_row)
{
	LineBuffer* old_line = tf->lines[current_row];
	LineBuffer* new_line = create_line_buffer();
	int split_length = old_line->line_length - old_line->gap_start;
	if (split_length > 0)
	{
		if (new_line->size < split_length + new_line->gap_size())
		{
			new_line->expand_buffer(split_length + new_line->gap_size());
		}
		memcpy(new_line->data + new_line->gap_start, old_line->data + old_line->gap_end, split_length);
		new_line->gap_start += split_length;
		new_line->line_length = split_length;

	}
	old_line->line_length = old_line->gap_start;
	old_line->gap_end = old_line->size;
	tf->lines.insert(tf->lines.begin() + current_row + 1, new_line);
	tf->cursor_col = 0;
	tf->cursor_row++;
}

void clean_memory(TextFile* tf)
{
	for(LineBuffer* line : tf->lines)
	{
		delete[] line->data;
		delete line;
	}
}

int main()
{
	HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	if(stdout_handle == INVALID_HANDLE_VALUE)
	{
		cerr << "Error getting stdout handle" << endl;
		return 1;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if(!GetConsoleScreenBufferInfo(stdout_handle, &csbi))
	{
		cerr << "Error getting console screen buffer info" << endl;
		return 1;
	};

	DWORD console_size = csbi.dwSize.X * csbi.dwSize.Y;
	COORD coord_screen = { 0, 0 };
	DWORD chars_written;

	FillConsoleOutputCharacter(stdout_handle, ' ', console_size, coord_screen, &chars_written);
	FillConsoleOutputAttribute(stdout_handle, csbi.wAttributes, console_size, coord_screen, &chars_written);
	SetConsoleCursorPosition(stdout_handle, coord_screen);


	HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
	if(stdin_handle == INVALID_HANDLE_VALUE)
	{
		cerr << "Error getting stdin handle" << endl;
		return 1;
	}
	LineBuffer* line = create_line_buffer();
	INPUT_RECORD input_record;
	DWORD events_read;
	int last_drawn_length = 0;
	const SHORT edit_row = 1;

	cout << "press 'Esc' to quit" << endl;
	bool running = true;

	while (running)
	{
		if (!ReadConsoleInput(stdin_handle, &input_record, 1, &events_read))
		{
			cerr << "Error reading console input" << endl;
			break;
		}
		if (input_record.EventType != KEY_EVENT) continue;

		const KEY_EVENT_RECORD& key_event = input_record.Event.KeyEvent;
		if (!key_event.bKeyDown) continue;

		bool redraw = false;
		char ch = key_event.uChar.AsciiChar;
			
			
		switch (key_event.wVirtualKeyCode)
		{
		case VK_ESCAPE:
			cout << "quitting..." << endl;
			running = false;
			break;
		case VK_LEFT:
			line->move_gap(line->gap_start - 1);
			redraw = true;
			break;
		case VK_RIGHT:
			line->move_gap(line->gap_start + 1);
			redraw = true;
			break;
		case VK_HOME:
			line->move_gap(0);
			redraw = true;
			break;
		case VK_END:
			line->move_gap(line->line_length);
			redraw = true;
			break;
		case VK_BACK:
			if (line->backspace_button()) redraw = true;
			break;
		case VK_DELETE:
			if (line->delete_button()) redraw = true;
			break;
		default:
			if(ch >= 32 && ch <= 127) //printable ASCII range
			{
				line->insert_character(ch);
				redraw = true;
			}
			break;
		}
		if (redraw)
		{
			redraw_line(stdout_handle, line, edit_row, last_drawn_length);
		}

	}
	delete[] line->data;
	delete line;
	return 0;


}

