#include "../EPD_raster.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

const size_t kBufferSize=400UL*300UL/8UL;
const size_t kGuardSize=32;
const size_t kStorageSize=kBufferSize+2*kGuardSize;
size_t readCount=0;
size_t yieldCount=0;

struct Model {
  const char* name;
  epd_raster::BufferLayout layout;
  int16_t xDot;
  int16_t yDot;
};

struct DrawCase {
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
};

uint8_t ReadRamByte(const uint8_t* address)
{
  readCount++;
  return *address;
}

void CountYield()
{
  yieldCount++;
}

void ReferenceSetPixel(const epd_raster::Target& target,int x,int y)
{
  if(target.layout==epd_raster::ROTATED_1BPP)
  {
    const int originalX=x;
    x=y;
    y=target.yDot-1-originalX;
    if(x>=0&&y>=0&&x<target.xDot&&y<target.yDot)
    {
      const size_t index=static_cast<size_t>(x)/8+
                         static_cast<size_t>(y)*target.xDot/8;
      if(index<target.bufferSize)
        target.buffer[index]&=static_cast<uint8_t>(~(0x80U>>(x&7)));
    }
    return;
  }

  if(target.layout==epd_raster::INTERLEAVED_2BPP)
  {
    if(x>=0&&y>=0&&x<target.xDot&&y<target.yDot)
    {
      const size_t index=static_cast<size_t>(x)/4+
                         static_cast<size_t>(y)*target.xDot/4;
      const uint8_t mask=target.frame==0
        ?static_cast<uint8_t>(0x80U>>((x&3)*2))
        :static_cast<uint8_t>(0x40U>>((x&3)*2));
      if(index<target.bufferSize)
        target.buffer[index]&=static_cast<uint8_t>(~mask);
    }
    return;
  }

  if(x>=0&&y>=0&&x<target.xDot&&y<target.yDot)
  {
    const size_t index=static_cast<size_t>(x)/8+
                       static_cast<size_t>(y)*target.xDot/8;
    if(index<target.bufferSize)
      target.buffer[index]&=static_cast<uint8_t>(~(0x80U>>(x&7)));
  }
}

void ReferenceDrawXbm(const epd_raster::Target& target,const DrawCase& draw,
                      uint8_t scale,const std::vector<uint8_t>& source,
                      bool updateCursorAt2x,int16_t& cursor)
{
  if(draw.width<=0||draw.height<=0) return;
  const int sourceStride=(draw.height+7)/8;
  uint8_t data=0;
  for(int sourceX=0;sourceX<draw.width;sourceX++)
  {
    for(int sourceY=0;sourceY<draw.height;sourceY++)
    {
      if((sourceY&7)!=0) data=static_cast<uint8_t>(data<<1);
      else data=source[static_cast<size_t>(sourceX)*sourceStride+sourceY/8];
      if((data&0x80)==0) continue;

      if(scale==1)
      {
        ReferenceSetPixel(target,draw.x+sourceY,draw.y+sourceX);
        cursor=static_cast<int16_t>(sourceX);
      }
      else if(scale==2)
      {
        const int x=draw.x+sourceY*2;
        const int y=draw.y+sourceX*2;
        ReferenceSetPixel(target,x,y);
        ReferenceSetPixel(target,x+1,y);
        ReferenceSetPixel(target,x,y+1);
        ReferenceSetPixel(target,x+1,y+1);
        if(updateCursorAt2x) cursor=static_cast<int16_t>(sourceX*2);
      }
    }
  }
}

std::string Describe(const Model& model,const DrawCase& draw,uint8_t frame,
                     uint8_t scale,bool updateCursorAt2x,int iteration)
{
  std::ostringstream output;
  output<<model.name<<" frame="<<static_cast<int>(frame)
        <<" scale="<<static_cast<int>(scale)
        <<" cursor2x="<<(updateCursorAt2x?"RAM":"PROGMEM")
        <<" draw=("<<draw.x<<','<<draw.y<<','<<draw.width<<','<<draw.height
        <<") iteration="<<iteration;
  return output.str();
}

void Require(bool condition,const std::string& message)
{
  if(condition) return;
  std::cerr<<"FAILED: "<<message<<std::endl;
  std::exit(1);
}

void RunOneDraw(const Model& model,const DrawCase& draw,uint8_t frame,
                uint8_t scale,bool updateCursorAt2x,
                const std::vector<uint8_t>& source,
                std::vector<uint8_t>& referenceBuffer,
                std::vector<uint8_t>& optimizedBuffer,
                int16_t& referenceCursor,int16_t& optimizedCursor,
                int iteration)
{
  epd_raster::Target referenceTarget={
    referenceBuffer.data()+kGuardSize,kBufferSize,model.xDot,model.yDot,
    model.layout,frame
  };
  epd_raster::Target optimizedTarget={
    optimizedBuffer.data()+kGuardSize,kBufferSize,model.xDot,model.yDot,
    model.layout,frame
  };
  const std::vector<uint8_t> before=optimizedBuffer;

  ReferenceDrawXbm(referenceTarget,draw,scale,source,updateCursorAt2x,
                   referenceCursor);
  readCount=0;
  yieldCount=0;
  const epd_raster::DrawResult result=epd_raster::DrawXbm(
    optimizedTarget,draw.x,draw.y,draw.width,draw.height,scale,source.data(),
    ReadRamByte,CountYield);
  if(result.hasInk&&(scale==1||(scale==2&&updateCursorAt2x)))
    optimizedCursor=result.cursor;

  const std::string context=Describe(model,draw,frame,scale,updateCursorAt2x,
                                     iteration);
  Require(referenceBuffer==optimizedBuffer,"framebuffer mismatch: "+context);
  Require(referenceCursor==optimizedCursor,"cursor mismatch: "+context);
  const size_t expectedReads=static_cast<size_t>(draw.width)*
                             ((draw.height+7)/8);
  const size_t expectedYields=static_cast<size_t>((draw.width+7)/8);
  Require(readCount==expectedReads,"source read count mismatch: "+context);
  Require(yieldCount==expectedYields,"yield count mismatch: "+context);

  if(model.layout==epd_raster::INTERLEAVED_2BPP)
  {
    const uint8_t otherPlaneMask=frame==0?0x55:0xaa;
    for(size_t index=0;index<optimizedBuffer.size();index++)
    {
      const uint8_t changed=static_cast<uint8_t>(before[index]^optimizedBuffer[index]);
      Require((changed&otherPlaneMask)==0,
              "non-selected bit plane changed: "+context);
    }
  }
}

std::vector<uint8_t> MakeSource(const DrawCase& draw,std::mt19937& random,
                                int pattern)
{
  const size_t sourceSize=draw.width>0&&draw.height>0
    ?static_cast<size_t>(draw.width)*((draw.height+7)/8):1;
  std::vector<uint8_t> source(sourceSize);
  if(pattern==0)
    std::fill(source.begin(),source.end(),static_cast<uint8_t>(0x00));
  else if(pattern==1)
    std::fill(source.begin(),source.end(),static_cast<uint8_t>(0xff));
  else
    for(size_t index=0;index<source.size();index++)
      source[index]=static_cast<uint8_t>(random());
  return source;
}

DrawCase RandomCase(const Model& model,std::mt19937& random)
{
  std::uniform_int_distribution<int> widthDistribution(1,96);
  std::uniform_int_distribution<int> heightDistribution(1,104);
  const int width=widthDistribution(random);
  const int height=heightDistribution(random);
  const int logicalXLimit=model.layout==epd_raster::ROTATED_1BPP
    ?model.yDot:model.xDot;
  const int logicalYLimit=model.layout==epd_raster::ROTATED_1BPP
    ?model.xDot:model.yDot;
  std::uniform_int_distribution<int> xDistribution(-height-8,logicalXLimit+8);
  std::uniform_int_distribution<int> yDistribution(-width-8,logicalYLimit+8);
  return DrawCase{
    static_cast<int16_t>(xDistribution(random)),
    static_cast<int16_t>(yDistribution(random)),
    static_cast<int16_t>(width),
    static_cast<int16_t>(height)
  };
}

} // namespace

int main()
{
  const Model models[]={
    {"WX29",epd_raster::DIRECT_1BPP,128,296},
    {"WF29",epd_raster::DIRECT_1BPP,128,296},
    {"OPM42",epd_raster::ROTATED_1BPP,400,300},
    {"WF58",epd_raster::DIRECT_1BPP,648,480},
    {"WF29BZ03",epd_raster::INTERLEAVED_2BPP,128,296},
    {"C154",epd_raster::DIRECT_1BPP,152,152},
    {"DKE42_3COLOR",epd_raster::ROTATED_1BPP,400,300},
    {"DKE29_3COLOR",epd_raster::DIRECT_1BPP,128,296},
    {"WF42",epd_raster::ROTATED_1BPP,400,300},
    {"WF32",epd_raster::INTERLEAVED_2BPP,200,300},
  };
  std::mt19937 random(0x5eed1234U);
  int verifiedDraws=0;

  for(const Model& model:models)
  {
    const uint8_t frames[]={0,1,2,255};
    const int frameCount=model.layout==epd_raster::INTERLEAVED_2BPP?4:1;
    const int logicalXLimit=model.layout==epd_raster::ROTATED_1BPP
      ?model.yDot:model.xDot;
    const int logicalYLimit=model.layout==epd_raster::ROTATED_1BPP
      ?model.xDot:model.yDot;
    const DrawCase boundaries[]={
      {0,0,1,1},
      {-9,-7,13,17},
      {static_cast<int16_t>(logicalXLimit-3),
       static_cast<int16_t>(logicalYLimit-5),17,19},
      {static_cast<int16_t>(logicalXLimit+2),
       static_cast<int16_t>(logicalYLimit+3),9,11},
      {5,7,35,72},
      {13,19,80,80},
      {0,184,33,65},
    };

    for(int frameIndex=0;frameIndex<frameCount;frameIndex++)
    {
      const uint8_t frame=frames[frameIndex];
      for(uint8_t scale=1;scale<=2;scale++)
      {
        for(int cursorMode=0;cursorMode<2;cursorMode++)
        {
          const bool updateCursorAt2x=cursorMode==0;
          for(size_t boundary=0;
              boundary<sizeof(boundaries)/sizeof(boundaries[0]);boundary++)
          {
            std::vector<uint8_t> initial(kStorageSize);
            for(size_t index=0;index<initial.size();index++)
              initial[index]=static_cast<uint8_t>(random());
            std::vector<uint8_t> referenceBuffer=initial;
            std::vector<uint8_t> optimizedBuffer=initial;
            int16_t referenceCursor=static_cast<int16_t>(random());
            int16_t optimizedCursor=referenceCursor;
            const std::vector<uint8_t> source=MakeSource(
              boundaries[boundary],random,static_cast<int>((boundary+1)%3));
            RunOneDraw(model,boundaries[boundary],frame,
                       scale,updateCursorAt2x,source,referenceBuffer,
                       optimizedBuffer,referenceCursor,optimizedCursor,
                       verifiedDraws++);
          }

          // Ink stops before the final source columns. CurrentCursor must point
          // at the last inked column, not simply width-1.
          {
            const DrawCase trailingBlank={1,1,9,9};
            std::vector<uint8_t> source(
              static_cast<size_t>(trailingBlank.width)*2,
              static_cast<uint8_t>(0x00));
            source[2]=0x80;
            source[8]=0x40;
            std::vector<uint8_t> initial(kStorageSize);
            for(size_t index=0;index<initial.size();index++)
              initial[index]=static_cast<uint8_t>(random());
            std::vector<uint8_t> referenceBuffer=initial;
            std::vector<uint8_t> optimizedBuffer=initial;
            int16_t referenceCursor=-123;
            int16_t optimizedCursor=referenceCursor;
            RunOneDraw(model,trailingBlank,frame,scale,updateCursorAt2x,
                       source,referenceBuffer,optimizedBuffer,referenceCursor,
                       optimizedCursor,verifiedDraws++);
          }

          for(int iteration=0;iteration<60;iteration++)
          {
            std::vector<uint8_t> initial(kStorageSize);
            for(size_t index=0;index<initial.size();index++)
              initial[index]=static_cast<uint8_t>(random());
            std::vector<uint8_t> referenceBuffer=initial;
            std::vector<uint8_t> optimizedBuffer=initial;
            int16_t referenceCursor=static_cast<int16_t>(random());
            int16_t optimizedCursor=referenceCursor;

            const DrawCase first=RandomCase(model,random);
            const DrawCase second=RandomCase(model,random);
            const std::vector<uint8_t> firstSource=MakeSource(first,random,iteration%3);
            const std::vector<uint8_t> secondSource=MakeSource(second,random,(iteration+1)%3);
            RunOneDraw(model,first,frame,scale,
                       updateCursorAt2x,firstSource,referenceBuffer,
                       optimizedBuffer,referenceCursor,optimizedCursor,
                       verifiedDraws++);
            RunOneDraw(model,second,frame,scale,
                       updateCursorAt2x,secondSource,referenceBuffer,
                       optimizedBuffer,referenceCursor,optimizedCursor,
                       verifiedDraws++);
          }
        }
      }
    }

    if(model.layout==epd_raster::INTERLEAVED_2BPP)
    {
      // Draw old/new content into different planes without clearing in between.
      // Each pass must preserve both the initial pixels and the other plane.
      for(uint8_t scale=1;scale<=2;scale++)
      {
        for(int cursorMode=0;cursorMode<2;cursorMode++)
        {
          const bool updateCursorAt2x=cursorMode==0;
          const DrawCase first={-3,5,35,72};
          const DrawCase second={4,11,35,72};
          const std::vector<uint8_t> firstSource=MakeSource(first,random,2);
          const std::vector<uint8_t> secondSource=MakeSource(second,random,2);
          std::vector<uint8_t> initial(kStorageSize);
          for(size_t index=0;index<initial.size();index++)
            initial[index]=static_cast<uint8_t>(random());
          std::vector<uint8_t> referenceBuffer=initial;
          std::vector<uint8_t> optimizedBuffer=initial;
          int16_t referenceCursor=71;
          int16_t optimizedCursor=referenceCursor;
          RunOneDraw(model,first,0,scale,updateCursorAt2x,firstSource,
                     referenceBuffer,optimizedBuffer,referenceCursor,
                     optimizedCursor,verifiedDraws++);
          RunOneDraw(model,second,1,scale,updateCursorAt2x,secondSource,
                     referenceBuffer,optimizedBuffer,referenceCursor,
                     optimizedCursor,verifiedDraws++);
        }
      }
    }
  }

  std::cout<<"PASS: "<<verifiedDraws
           <<" XBM draws matched the original pixel renderer byte-for-byte"
           <<std::endl;
  return 0;
}
