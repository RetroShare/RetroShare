#ifndef RS_TERM_SERVER_H
#define RS_TERM_SERVER_H

class RsTermServer
{
public:
	/* this must be regularly ticked to update the display */
	virtual void reset() = 0; 
	virtual int tick(bool haveInput, char keypress, std::string &output) = 0;
};


#endif // RS_TERM_SERVER_H
