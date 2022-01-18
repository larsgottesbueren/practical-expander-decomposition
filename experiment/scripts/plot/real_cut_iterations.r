suppressMessages(library(tidyverse))
suppressMessages(library(patchwork))
suppressMessages(library(cowplot))

args <- commandArgs(trailingOnly=TRUE)

if (length(args) != 2)
    stop("Expected two arguments")

input_file <- args[1]
output_file <- args[2]

df <- read.csv(input_file) %>%
    group_by(graph,type,targetbalance,phi) %>%
    summarize(iterationsratio=sum(iterations[strategy=="default"])/sum(iterations[strategy=="balanced"]))

plotWithData <- function(data) {
    ggplot(data, aes(x=reorder(factor(type), iterationsratio),
                     y=iterationsratio,
                     fill=ifelse(iterationsratio < 1, "Original", "Balanced"))) +
        geom_bar(stat="identity") +
        geom_hline(yintercept=1) +
        labs(y="Ratio",
             x="",
             fill="Which is better?") +
        theme(axis.text.x = element_text(size = 4, angle = 90, vjust = 0.5, hjust = 1)) +
        scale_fill_manual(values = c('Original' = "blue", 'Balanced' = "red"))
}

p1 <- plotWithData(filter(df, near(targetbalance, 0.0), near(phi, 0.005)))
p2 <- plotWithData(filter(df, near(targetbalance, 0.25), near(phi, 0.005)))
p3 <- plotWithData(filter(df, near(targetbalance, 0.45), near(phi, 0.005)))

# See https://github.com/wilkelab/cowplot/blob/master/vignettes/shared_legends.Rmd
legend <- get_legend(p3 + theme(legend.justification = "top"))

plot <- plot_grid(
    p1 + theme(legend.position="none"),
    p2 + theme(legend.position="none"),
    p3 + theme(legend.position="none"),
    legend,
    labels=c('a','b','c')
)

ggsave(output_file, plot)
