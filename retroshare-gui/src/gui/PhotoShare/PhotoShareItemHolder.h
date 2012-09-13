#ifndef PHOTOSHAREITEMHOLDER_H
#define PHOTOSHAREITEMHOLDER_H

class PhotoShareItem
{

public:
    PhotoShareItem() { return; }
    virtual ~PhotoShareItem(){ return; }
    virtual bool isSelected() = 0;
    virtual void setSelected(bool selected) = 0;
};

class PhotoShareItemHolder
{
public:
    PhotoShareItemHolder();

    virtual void notifySelection(PhotoShareItem* selection) = 0;
};

#endif // PHOTOSHAREITEMHOLDER_H
