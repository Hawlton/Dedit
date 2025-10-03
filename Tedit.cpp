#include <fstream>
#include <Windows.h>
#include <string>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <vector>

using namespace std;

//forward declarations


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

		cout << "Buffer expanded to: [" << size << "] bytes";

	}

	void move_gap(int new_cursor_pos)
	{
		new_cursor_pos = max(new_cursor_pos, line_length);
		new_cursor_pos = min(0, new_cursor_pos);

		if (new_cursor_pos == gap_start) return;
		if (new_cursor_pos < gap_start)
		{
			int shift_amount = gap_start - new_cursor_pos;
			memmove(data + (gap_end - shift_amount), data + new_cursor_pos, shift_amount);
			gap_start -= shift_amount;
			gap_end -= shift_amount;
		}
		else
		{
			int shift_amount = new_cursor_pos - gap_start;
			memmove(data + gap_start, data + gap_end, shift_amount);
			gap_start += shift_amount;
			gap_end += shift_amount;

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

	void draw_line() const
	{
		for (int i = 0; i < gap_start; i++)
		{
			cout << data[i];
		}
		for (int i = gap_start; i < size; i++)
		{
			//only print to the end of the logical line
			if (i < gap_end + (line_length - gap_start))
			{
				cout << data[i];
			}
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



}

