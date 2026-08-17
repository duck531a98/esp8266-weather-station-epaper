#ifndef EPD_RASTER_H
#define EPD_RASTER_H

#include <stddef.h>
#include <stdint.h>

namespace epd_raster {

enum BufferLayout : uint8_t {
  DIRECT_1BPP = 0,
  ROTATED_1BPP = 1,
  INTERLEAVED_2BPP = 2,
};

struct Target {
  uint8_t* buffer;
  size_t bufferSize;
  int xDot;
  int yDot;
  BufferLayout layout;
  uint8_t frame;
};

struct DrawResult {
  bool hasInk;
  int16_t cursor;
};

typedef uint8_t (*ReadByte)(const uint8_t* address);
typedef void (*YieldCallback)();

inline uint8_t TopBitMask(uint8_t count)
{
  if(count==0) return 0;
  if(count>=8) return 0xff;
  return static_cast<uint8_t>(0xffU<<(8-count));
}

// Spread four adjacent pixels to the even bits of one interleaved byte.
inline uint8_t SpreadNibble(uint8_t value)
{
  value&=0x0f;
  value=static_cast<uint8_t>((value|(value<<2))&0x33);
  value=static_cast<uint8_t>((value|(value<<1))&0x55);
  return value;
}

// Duplicate each of four pixels so a 1x source can be rendered at 2x scale.
inline uint8_t DoubleNibble(uint8_t value)
{
  const uint8_t spread=SpreadNibble(value);
  return static_cast<uint8_t>(spread|(spread<<1));
}

inline uint16_t DoubleByte(uint8_t value)
{
  return static_cast<uint16_t>(
    (static_cast<uint16_t>(DoubleNibble(value>>4))<<8)|
    DoubleNibble(value));
}

// Pixels are held MSB-first in a 16-bit value. Extract a short consecutive
// segment and return it right-aligned.
inline uint8_t ExtractSegment(uint16_t pixels,uint8_t offset,uint8_t count)
{
  const uint16_t shifted=static_cast<uint16_t>(
    static_cast<uint32_t>(pixels)<<offset);
  return static_cast<uint8_t>(shifted>>(16-count));
}

// Merge black pixels into a one-bit row. Source zeroes are transparent: the
// destination is only AND-cleared, never assigned, so earlier drawing remains.
inline void BlendMonoRow(const Target& target,int row,int startX,
                         uint16_t pixels,uint8_t count)
{
  if(target.buffer==nullptr||target.xDot<=0||target.yDot<=0||count==0||
     row<0||row>=target.yDot)
    return;

  const int endX=startX+count;
  if(endX<=0||startX>=target.xDot) return;

  int pixel=startX<0?0:startX;
  const int clippedEnd=endX>target.xDot?target.xDot:endX;
  uint8_t sourceOffset=static_cast<uint8_t>(pixel-startX);
  const size_t rowBytes=static_cast<size_t>(target.xDot)/8;

  while(pixel<clippedEnd)
  {
    const uint8_t byteOffset=static_cast<uint8_t>(pixel&7);
    const int available=8-byteOffset;
    const uint8_t segmentCount=static_cast<uint8_t>(
      clippedEnd-pixel<available?clippedEnd-pixel:available);
    const uint8_t sourceBits=ExtractSegment(pixels,sourceOffset,segmentCount);
    const uint8_t mask=static_cast<uint8_t>(
      sourceBits<<(8-byteOffset-segmentCount));
    const size_t index=static_cast<size_t>(row)*rowBytes+
                       static_cast<size_t>(pixel)/8;
    if(mask!=0&&index<target.bufferSize)
      target.buffer[index]&=static_cast<uint8_t>(~mask);
    pixel+=segmentCount;
    sourceOffset=static_cast<uint8_t>(sourceOffset+segmentCount);
  }
}

// WF29BZ03/WF32 store two image planes in alternating bits. Only the selected
// plane is cleared; the other plane and transparent source pixels are retained.
inline void BlendInterleavedRow(const Target& target,int row,int startX,
                                uint16_t pixels,uint8_t count)
{
  if(target.buffer==nullptr||target.xDot<=0||target.yDot<=0||count==0||
     row<0||row>=target.yDot)
    return;

  const int endX=startX+count;
  if(endX<=0||startX>=target.xDot) return;

  int pixel=startX<0?0:startX;
  const int clippedEnd=endX>target.xDot?target.xDot:endX;
  uint8_t sourceOffset=static_cast<uint8_t>(pixel-startX);
  const size_t rowBytes=static_cast<size_t>(target.xDot)/4;
  const uint8_t planeShift=target.frame==0?1:0;

  while(pixel<clippedEnd)
  {
    const uint8_t byteOffset=static_cast<uint8_t>(pixel&3);
    const int available=4-byteOffset;
    const uint8_t segmentCount=static_cast<uint8_t>(
      clippedEnd-pixel<available?clippedEnd-pixel:available);
    const uint8_t sourceBits=ExtractSegment(pixels,sourceOffset,segmentCount);
    const uint8_t nibble=static_cast<uint8_t>(
      sourceBits<<(4-byteOffset-segmentCount));
    const uint8_t mask=static_cast<uint8_t>(SpreadNibble(nibble)<<planeShift);
    const size_t index=static_cast<size_t>(row)*rowBytes+
                       static_cast<size_t>(pixel)/4;
    if(mask!=0&&index<target.bufferSize)
      target.buffer[index]&=static_cast<uint8_t>(~mask);
    pixel+=segmentCount;
    sourceOffset=static_cast<uint8_t>(sourceOffset+segmentCount);
  }
}

inline DrawResult DrawDirectXbm(const Target& target,int16_t xMove,int16_t yMove,
                                int16_t width,int16_t height,uint8_t scale,
                                const uint8_t* xbm,ReadByte readByte,
                                YieldCallback yieldCallback)
{
  DrawResult result={false,0};
  const size_t sourceStride=static_cast<size_t>((height+7)/8);

  for(int sourceX=0;sourceX<width;sourceX++)
  {
    bool columnHasInk=false;
    const int firstRow=yMove+sourceX*scale;
    for(int sourceY=0;sourceY<height;sourceY+=8)
    {
      const uint8_t pixelCount=static_cast<uint8_t>(
        height-sourceY<8?height-sourceY:8);
      uint8_t data=readByte(xbm+static_cast<size_t>(sourceX)*sourceStride+
                           static_cast<size_t>(sourceY/8));
      data&=TopBitMask(pixelCount);
      if(data==0) continue;

      columnHasInk=true;
      if(scale==1)
      {
        const uint16_t pixels=static_cast<uint16_t>(data)<<8;
        const int firstPixel=xMove+sourceY;
        if(target.layout==INTERLEAVED_2BPP)
          BlendInterleavedRow(target,firstRow,firstPixel,pixels,pixelCount);
        else
          BlendMonoRow(target,firstRow,firstPixel,pixels,pixelCount);
      }
      else
      {
        const uint16_t pixels=DoubleByte(data);
        const uint8_t scaledCount=static_cast<uint8_t>(pixelCount*2);
        const int firstPixel=xMove+sourceY*2;
        if(target.layout==INTERLEAVED_2BPP)
        {
          BlendInterleavedRow(target,firstRow,firstPixel,pixels,scaledCount);
          BlendInterleavedRow(target,firstRow+1,firstPixel,pixels,scaledCount);
        }
        else
        {
          BlendMonoRow(target,firstRow,firstPixel,pixels,scaledCount);
          BlendMonoRow(target,firstRow+1,firstPixel,pixels,scaledCount);
        }
      }
    }

    if(columnHasInk)
    {
      result.hasInk=true;
      result.cursor=static_cast<int16_t>(sourceX*scale);
    }
    if(yieldCallback!=nullptr&&(sourceX&7)==0) yieldCallback();
  }
  return result;
}

inline DrawResult DrawRotatedXbm(const Target& target,int16_t xMove,int16_t yMove,
                                 int16_t width,int16_t height,uint8_t scale,
                                 const uint8_t* xbm,ReadByte readByte,
                                 YieldCallback yieldCallback)
{
  DrawResult result={false,0};
  const size_t sourceStride=static_cast<size_t>((height+7)/8);

  // Eight source columns become one (or two at 2x) destination bytes after
  // rotation. Transpose one 8x8 source block into row masks before blending.
  for(int sourceXStart=0;sourceXStart<width;sourceXStart+=8)
  {
    const uint8_t columnCount=static_cast<uint8_t>(
      width-sourceXStart<8?width-sourceXStart:8);

    for(int sourceYStart=0;sourceYStart<height;sourceYStart+=8)
    {
      const uint8_t rowCount=static_cast<uint8_t>(
        height-sourceYStart<8?height-sourceYStart:8);
      uint16_t rowPixels[8]={0,0,0,0,0,0,0,0};

      for(uint8_t column=0;column<columnCount;column++)
      {
        const int sourceX=sourceXStart+column;
        uint8_t data=readByte(xbm+static_cast<size_t>(sourceX)*sourceStride+
                              static_cast<size_t>(sourceYStart/8));
        data&=TopBitMask(rowCount);
        if(data==0) continue;

        const int cursor=sourceX*scale;
        if(!result.hasInk||cursor>result.cursor)
          result.cursor=static_cast<int16_t>(cursor);
        result.hasInk=true;

        const uint16_t columnMask=static_cast<uint16_t>(0x8000U>>column);
        for(uint8_t row=0;row<rowCount;row++)
        {
          if((data&(0x80U>>row))!=0) rowPixels[row]|=columnMask;
        }
      }

      for(uint8_t row=0;row<rowCount;row++)
      {
        if(rowPixels[row]==0) continue;
        const int logicalX=xMove+(sourceYStart+row)*scale;
        const int targetRow=target.yDot-1-logicalX;
        const int firstPixel=yMove+sourceXStart*scale;
        if(scale==1)
        {
          BlendMonoRow(target,targetRow,firstPixel,rowPixels[row],columnCount);
        }
        else
        {
          const uint8_t sourcePixels=static_cast<uint8_t>(rowPixels[row]>>8);
          const uint16_t pixels=DoubleByte(sourcePixels);
          const uint8_t scaledCount=static_cast<uint8_t>(columnCount*2);
          BlendMonoRow(target,targetRow,firstPixel,pixels,scaledCount);
          BlendMonoRow(target,targetRow-1,firstPixel,pixels,scaledCount);
        }
      }
    }

    if(yieldCallback!=nullptr) yieldCallback();
  }
  return result;
}

inline DrawResult DrawXbm(const Target& target,int16_t xMove,int16_t yMove,
                          int16_t width,int16_t height,uint8_t scale,
                          const uint8_t* xbm,ReadByte readByte,
                          YieldCallback yieldCallback)
{
  DrawResult result={false,0};
  if(target.buffer==nullptr||xbm==nullptr||readByte==nullptr||
     width<=0||height<=0)
    return result;

  if(scale!=1&&scale!=2)
  {
    const size_t sourceStride=static_cast<size_t>((height+7)/8);
    for(int sourceX=0;sourceX<width;sourceX++)
    {
      for(int sourceY=0;sourceY<height;sourceY+=8)
        readByte(xbm+static_cast<size_t>(sourceX)*sourceStride+
                 static_cast<size_t>(sourceY/8));
      if(yieldCallback!=nullptr&&(sourceX&7)==0) yieldCallback();
    }
    return result;
  }

  if(target.layout==ROTATED_1BPP)
    return DrawRotatedXbm(target,xMove,yMove,width,height,scale,xbm,readByte,
                          yieldCallback);
  return DrawDirectXbm(target,xMove,yMove,width,height,scale,xbm,readByte,
                       yieldCallback);
}

} // namespace epd_raster

#endif
