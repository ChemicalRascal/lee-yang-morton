library(ggplot2)
library(scales)
library(ggthemr)
library(plyr)

data <- read.csv(file="allCountries.2mr.rprep.csv",sep=",")
#data$time_per_posting <- data$time_ns / data$postings
#data$space_per_posting <- data$space_bits / data$postings
#data$method <- reorder(data$method, as.numeric(-data$time_ns), FUN=sum)

palette <- c('#ff0033', '#1e65aa', '#b1152a', '#5da13b', '#58df8c', '#7f763d','#e08114', '#022114', '#127999', '#f87197', '#f87198', '#f87199')
names(palette) <- levels(data$method)

#tmpdata <- subset(data,data$method == "U32")
#total_postings <- sum(as.numeric(tmpdata$postings))


#sdata <- ddply(data, "method", summarise, time_per_posting = sum(as.numeric(time_ns))/total_postings, space_usage = sum(as.numeric(space_bits))/total_postings  )

custom_theme <- define_palette(
	swatch = c("#000000",palette), 
	gradient = c('#d0d0d0', '#000000'),
	background = "#ffffff",
	text = c('#000000', '#000000'),
	line = c('#000000', '#000000'),
	gridline = "#c3c3c3")


ggthemr(custom_theme,layout = "scientific",type = "inner",spacing = 0.5,text_size = 8, line_weight = 0.2)
p <- ggplot(data,aes(x=size,y=X0.01,shape=method,fill=method,color=method))
p <<- p + geom_point(aes(x=size,y=X0.02,shape=method,fill=method,color=method))
p <<- p + geom_point(aes(x=size,y=X0.03,shape=method,fill=method,color=method))
p <<- p + geom_point(aes(x=size,y=X0.04,shape=method,fill=method,color=method))
p <<- p + geom_point(aes(x=size,y=X0.05,shape=method,fill=method,color=method))
p <<- p + geom_point(aes(x=size,y=X0.06,shape=method,fill=method,color=method))
p <<- p + geom_point(aes(x=size,y=X0.07,shape=method,fill=method,color=method))
p <<- p + geom_point(aes(x=size,y=X0.08,shape=method,fill=method,color=method))
p <<- p + geom_point(aes(x=size,y=X0.09,shape=method,fill=method,color=method))
p <<- p + geom_point(aes(x=size,y=X0.10,shape=method,fill=method,color=method))
p <- p + geom_point(size=3)
p <- p + scale_y_continuous(name="Time per 8k queries [s]")#,breaks=seq(0,6,0.5))
#p <- p + scale_x_continuous(trans="log2",name="Bits per Posting",breaks=c(2,4,5,6,8,10,12,16,32,64))
#p <- p + scale_x_continuous(name="Memory consumed [b]",breaks=c(15360,2048,30720,40960,51200))
p <- p + scale_x_continuous(name="Memory consumed [MiB]",breaks=seq(1*1024*1024,15*1024*1024,1024*1024),limits=c(5*1024*1024,15*1024*1024),labels=seq(1,15))
#p <- p + scale_shape_manual(values=c(15,16,17,23,25,3,7,8,9,10,11,12,13))
p <- p + scale_shape_manual(values=c(4,4,4,4,4,4,4,4,4,4,4,4))
p <- p + scale_fill_manual(values=palette)
p <- p + scale_color_manual(values=palette)
p <- p + theme(plot.margin = unit(c(1,0,1,0), "mm"))
p <- p + theme(legend.position = c(0.79,0.76))
p <- p + theme(legend.title = element_blank())
p <- p + theme(legend.margin = margin(t = -2, r = 2, b = 2, l = 2, unit = "pt"))
p <- p + theme(legend.background = element_rect(color="black",linetype="dotted"))
p <- p + theme(legend.key.size = unit(0.4, "cm"))
p <- p + theme(axis.text.x= element_text(face="plain",size=7))
p <- p + theme(axis.text.y= element_text(face="plain",size=7))
p <- p + theme(axis.title.x= element_text(face="plain",size=8))
p <- p + theme(axis.title.y= element_text(face="plain",size=8))
##p <- p + theme(legend.key = element_rect(size = 1, color = 'white'))
p <- p + guides(fill=guide_legend(ncol=2),shape=guide_legend(ncol=2))
#p <- p + annotation_logticks(base=2,"b")
p <- p + ggtitle("Time/Space on allCountries.2mr")

figWidth=4.25
figHeight=2.50
filename <- "2m-time-space.png"
#ggsave(filename,plot=p,device=cairo_pdf,width=figWidth,height=figHeight,units=c("in"))
ggsave(filename,plot=p,device="png",width=figWidth,height=figHeight,units=c("in"))

#pdf(file="./time-space.pdf",width=figWidth,height=figHeight)
#print(p)
#dev.off()
